#ifdef _WIN32
#  include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <time.h>

#ifndef __APPLE__
#  include <GL/glut.h>
#else
#  include <GLUT/glut.h>
#endif
#include <AR/gsub.h>
#include <AR/param.h>
#include <AR/ar.h>
#include <AR/video.h>

#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "object.h"

#define COLLIDE_DIST 30000.0

/* Object Data */
char            *model_name = "Data/object_data2";
ObjectData_T    *object;
int             objectnum;

int             xsize, ysize;
int				thresh = 100;
int             count = 0;
char            *lastRecongResult;
long            lastScanTime;
/* set up the video format globals */

#ifdef _WIN32
char			*vconf = "Data\\WDM_camera_flipV.xml";
#else
char			*vconf = "v4l2src device=/dev/video0 use-fixed-fps=false ! ffmpegcolorspace ! video/x-raw-rgb,bpp=24 ! identity name=artoolkit sync=true ! fakesink";
#endif

char           *cparam_name    = "Data/camera_para.dat";
ARParam         cparam;

static void   init(void);
static void   cleanup(void);
static void   keyEvent( unsigned char key, int x, int y);
static void   mainLoop(void);
static int draw( ObjectData_T *object, int objectnum );
static int  draw_object( int obj_id, double gl_para[16] );

/*for network */
void sendResult(char* message);
void sendFile(char* filename);

int main(int argc, char **argv)
{
	//initialize applications
	glutInit(&argc, argv);
    init();
	
	arVideoCapStart();
    lastRecongResult = NULL;
	lastScanTime = 0;
	//start the main event loop
    argMainLoop( NULL, keyEvent, mainLoop );

	return 0;
}

static void   keyEvent( unsigned char key, int x, int y)   
{
    /* quit if the ESC key is pressed */
    if( key == 0x1b ) {
        printf("*** %f (frame/sec)\n", (double)count/arUtilTimer());
        cleanup();
        exit(0);
    }
}

int readSizeFromStdin(){
	char c;
    char buff[1024];
	int count=0;
	int eol=0;
	while((c = getchar()) != EOF){
		buff[count] = c;
		if(buff[count-1] == '\r' && buff[count] == '\n'){
			eol = 1;
			break;	  
		}
		count++;	  
	}
	if(eol){
		buff[count] = '\0';
		char s[5];
		char* pch = strchr(buff, '=');
		if(pch != NULL){
			strncpy(s,pch-4, 4);
		    s[4]='\0';
			printf("size is:");
			printf("%s", s);
			printf("\r\n");
			char str[40];
			strcpy(str, pch+1); 
			return atoi(str);
		}	  
	}
	return -1;
}

int readFromStdin(char* buf, int size){
	int count=0;
	char c;
	int s = size;
	while((c = getchar()) ==EOF ||s){
		*(buf+count) = c;
		count++;
		s--;  
	}
	*(buf+count) = '\0';
	//print("in function readFromStdin\n");
	return count;  
}

/* main loop */
static void mainLoop(void)
{
    ARUint8         *dataPtr;
    ARMarkerInfo    *marker_info;
    int             marker_num;
    int             i,j,k;

    /* grab a video frame */
	/*
    if( (dataPtr = (ARUint8 *)arVideoGetImage()) == NULL ) {
        arUtilSleep(2);
        return;
    }
	
    if( count == 0 ) arUtilTimerReset();  
    count++;
	*/
	while(1){
		int size = readSizeFromStdin();
		if(size<=0){ continue;}
		dataPtr = (ARUint8 *)malloc(size);
		if(dataPtr == NULL){ 
			printf("Error in memory allocate \n");  
			return;
		}
		int s = size;
		while(s){
			int c = readFromStdin(dataPtr+size-s, s);
			s-=c;
		}
		break;  
	}
	//for debug
	//int n = strlen(dataPtr);
	
	/*draw the video*/
    argDrawMode2D();
    argDispImage( dataPtr, 0,0 );

	glColor3f( 1.0, 0.0, 0.0 );
	glLineWidth(6.0);

	/* detect the markers in the video frame */ 
	if(arDetectMarker(dataPtr, thresh, 
		&marker_info, &marker_num) < 0 ) {
		printf("Error in arDetectMarker() \n");
		cleanup();
		free(dataPtr); 
		exit(0);
	}

	/*Non-use in server side
	for( i = 0; i < marker_num; i++ ) {
		argDrawSquare(marker_info[i].vertex,0,0);
	}
	*/

	/*free the used memory for the next allocation*/
	    free(dataPtr);

	/* check for known patterns */
    for( i = 0; i < objectnum; i++ ) {
		k = -1;
		for( j = 0; j < marker_num; j++ ) {
	        if( object[i].id == marker_info[j].id) {
				if(lastRecongResult == NULL){
					lastRecongResult = object[i].name;
					lastScanTime = time(NULL);  
				}
				long timeDiff = time(NULL)-lastScanTime;
				if((strcmp(lastRecongResult,object[i].name) ==0) && (timeDiff <10)){
				  //do nothing
				  printf("Timer not expired: %ld \n",timeDiff);
				}else{
					/* you've found a pattern */
					lastRecongResult = object[i].name;
					lastScanTime = time(NULL);
					printf("Found pattern: %s \n",object[i].name);
					/*send the result to client*/
					printf("Before create thread\n");
					pthread_t sendToPhone;
					char message[256];
					int length = strlen(object[i].name);
					strncpy(message,object[i].name,length);
					message[length]='\n';
					message[length+1]='\0';
					pthread_create(&sendToPhone, NULL, (void *) &sendResult, (char *) &message);
					pthread_join(sendToPhone, NULL);
					printf("After create thread\n");
				}
				//Non-used in server side
				//glColor3f( 0.0, 1.0, 0.0 );
				//argDrawSquare(marker_info[j].vertex,0,0);

				if( k == -1 ) k = j;
		        else /* make sure you have the best pattern (highest confidence factor) */
					if( marker_info[k].cf < marker_info[j].cf ) k = j;
			}
		}
		if( k == -1 ) {
			object[i].visible = 0;
			continue;
		}
		
		/* calculate the transform for each marker */
		if( object[i].visible == 0 ) {
			/*
            arGetTransMat(&marker_info[k],
                          object[i].marker_center, object[i].marker_width,
                          object[i].trans);
			*/
        }
        else {
			/*
            arGetTransMatCont(&marker_info[k], object[i].trans,
                          object[i].marker_center, object[i].marker_width,
                          object[i].trans);
			*/
        }
        object[i].visible = 1;
	}
	
	arVideoCapNext();
	
	/* draw the AR graphics */
	//Non-used in server side
    //draw( object, objectnum );

	/*swap the graphics buffers*/
	argSwapBuffers();

}

static void init( void )
{
	ARParam  wparam;

    /* open the video path */
    if( arVideoOpen( vconf ) < 0 ) exit(0);
    /* find the size of the window */
    if( arVideoInqSize(&xsize, &ysize) < 0 ) exit(0);
    printf("Image size (x,y) = (%d,%d)\n", xsize, ysize);

    /* set the initial camera parameters */
    if( arParamLoad(cparam_name, 1, &wparam) < 0 ) {
        printf("Camera parameter load error !!\n");
        exit(0);
    }
	//xsize=240; ysize=320;
	xsize=320; ysize=180;
    arParamChangeSize( &wparam, xsize, ysize, &cparam );
    arInitCparam( &cparam );
    printf("*** Camera Parameter ***\n");
    arParamDisp( &cparam );

	/* load in the object data - trained markers and associated bitmap files */
    if( (object=read_ObjData(model_name, &objectnum)) == NULL ) exit(0);
    printf("Objectfile num = %d\n", objectnum);

    /* open the graphics window */
    argInit( &cparam, 2.0, 0, 0, 0, 0 );
}

/* cleanup function called when program exits */
static void cleanup(void)
{
	arVideoCapStop();
    arVideoClose();
    argCleanup();
}

/* draw the the AR objects */
static int draw( ObjectData_T *object, int objectnum )
{
    int     i;
    double  gl_para[16];
       
	glClearDepth( 1.0 );
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_LIGHTING);

    /* calculate the viewing parameters - gl_para */
    for( i = 0; i < objectnum; i++ ) {
        if( object[i].visible == 0 ) continue;
        argConvGlpara(object[i].trans, gl_para);
        draw_object( object[i].id, gl_para);
    }
     
	glDisable( GL_LIGHTING );
    glDisable( GL_DEPTH_TEST );
	
    return(0);
}

/* draw the user object */
static int  draw_object( int obj_id, double gl_para[16])
{
    GLfloat   mat_ambient[]				= {0.0, 0.0, 1.0, 1.0};
	GLfloat   mat_ambient_collide[]     = {1.0, 0.0, 0.0, 1.0};
    GLfloat   mat_flash[]				= {0.0, 0.0, 1.0, 1.0};
	GLfloat   mat_flash_collide[]       = {1.0, 0.0, 0.0, 1.0};
    GLfloat   mat_flash_shiny[] = {50.0};
    GLfloat   light_position[]  = {100.0,-200.0,200.0,0.0};
    GLfloat   ambi[]            = {0.1, 0.1, 0.1, 0.1};
    GLfloat   lightZeroColor[]  = {0.9, 0.9, 0.9, 0.1};
 
    argDrawMode3D();
    argDraw3dCamera( 0, 0 );
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd( gl_para );

 	/* set the material */
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambi);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);

    glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny);	

	if(obj_id == 0){
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash_collide);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient_collide);
		/* draw a cube */
		glTranslatef( 0.0, 0.0, 30.0 );
		glutSolidSphere(30,12,6);
	}
	else {
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		/* draw a cube */
		glTranslatef( 0.0, 0.0, 30.0 );
		glutSolidCube(60);
	}

    argDrawMode2D();

    return 0;
}

/*send the recognize result to client*/
void sendResult(char* msg){
	int sockfd,n;
	struct sockaddr_in server_address,clint_address;
	printf("Want to Write:%s\n", msg);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd <0){
		printf("Error in open socket\n");
		//return NULL;
	}
	bzero(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr("192.168.11.3");
	server_address.sin_port = htons(8888);
	if(connect(sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) <0){
		printf("Error in connecting\n");
		//return NULL;
	}
	sendto(sockfd, msg, strlen(msg), 0,(struct sockaddr *) &server_address, sizeof(server_address));
	close(sockfd);
	//return NULL;
}

/*send the pattern file to client*/
void sendFile(char* filename){
	int sockfd,n;
	struct sockaddr_in server_address,clint_address;
		
	char* path = "/home/hscc/MyARProject/ARToolKit/bin/Data/";
	char send_buf[2048]; /* max chunk size for sending file */
	int f; /* file handle for reading local file*/
	ssize_t read_bytes; /* bytes read from local file */
	ssize_t sent_bytes; /* bytes sent to connected socket */
 
	printf("Want to Send:%s\n", filename);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd <0){
		printf("Error in open socket\n");
		//return NULL;
	}
	bzero(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr("192.168.11.3");
	server_address.sin_port = htons(6666);
	if(connect(sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) <0){
		printf("Error in connecting\n");
		//return NULL;
	}
	char fs_name[256];
	strcpy (fs_name, path);
	strcpy (fs_name, filename);
	if( (f = open(fs_name, O_RDONLY)) < 0)
	{
		/* can't open requested file */
        printf("File %s Cannot be opened file on server.\n", fs_name);
	}else {
		/* open file successful */
		printf("Sending file: %s\n", fs_name);
		//send file name
		sendto(sockfd, filename, strlen(filename), 0,(struct sockaddr *) &server_address, sizeof(server_address));
		//sent file data
		while( (read_bytes = read(f, send_buf, 2048)) > 0 ){
			if( (sent_bytes = send(sockfd, send_buf, read_bytes, 0)) < read_bytes ){
				printf("send error\n");
			}
		}
	}
	close(sockfd);
	
}
