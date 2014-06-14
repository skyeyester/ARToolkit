#ifndef __UTIL__
#define __UTIL__

int readall(int connfd, string *input){
	while(1) {
		char buf[2];
		int n = read(connfd, &buf[0], 1);
		//cout << n << ":: ";
		buf[1]='\0';
		if(n==1) {
			int t = buf[0];
			//cout << "\t R::" << buf << "int:" << t << endl;
			input->append(buf, 1);
		}
		else if(errno == EAGAIN) return input->size();
		else return n;
	}	
}

char readone(int connfd, char* ch){
	char buf[2];
	int n = read(connfd, &buf[0], 1);
	if(n == 1){
		*ch = buf[0];
		return 1;
	}
	else if(errno == EAGAIN) return n;
	//else if(n==0) return 0;
	else return n;
	
}

#endif
