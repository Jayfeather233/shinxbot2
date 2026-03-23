// Found this code in my old computer's backup files.
// I can't remember when I wrote this...

#include <iostream>
#include <cstdio>
using namespace std;
int col[100][100],row[100][100];
int lc[100],lr[100];
int r,c;
int ans;

int mapx[100][100];
int dlc[100],dlr[100];
void opt(int x,int y){
	//printf("%d %d\n",x,y);
	for(int i=1;i<=r;i++){
		for(int j=1;j<=c;j++){
			printf("%d ",mapx[i][j]);
		}
		printf("\n");
	}
	printf("\n");
}
void dfs(int x,int y){
	//opt(x,y);
	if(x==r+1){
		opt(x,y);
		ans++;
		return;
	}
	if(x==r){
		if(!(dlc[y-1]==lc[y-1]&&col[y-1][lc[y-1]]==0)){
			//printf("re 1\n");
			return;
		}
	}
	if(y==c+1){
		if(dlr[x]==lr[x]&&row[x][lr[x]]==0){
			dfs(x+1,1);
		}else{
			//printf("re 2\n");
			;
		}
		return;
	}
	if((!mapx[x][y-1]||row[x][dlr[x]]==0)&&
		(!mapx[x-1][y]||col[y][dlc[y]]==0))
		dfs(x,y+1);
	
	if(mapx[x][y-1]&&row[x][dlr[x]]==0){
		//printf("re 3\n");
		return;
	}
	if(mapx[x-1][y]&&col[y][dlc[y]]==0){
		//printf("re 4\n");
		return;
	}
	if(!mapx[x][y-1]&&dlr[x]==lr[x]){
		//printf("re 5\n");
		return;
	}
	if(!mapx[x-1][y]&&dlc[y]==lc[y]){
		//printf("re 6\n");
		return;
	}
	
	
	mapx[x][y]=1;
	if(!mapx[x][y-1]) dlr[x]++;
	if(!mapx[x-1][y]) dlc[y]++;
	
	col[y][dlc[y]]--;
	row[x][dlr[x]]--;
	
	dfs(x,y+1);
	
	col[y][dlc[y]]++;
	row[x][dlr[x]]++;
	if(!mapx[x][y-1]) dlr[x]--;
	if(!mapx[x-1][y]) dlc[y]--;
	mapx[x][y]=0;
}
int main(){
	cin>>r>>c;
	for(int i=1;i<=r;i++){
		scanf("%d",&lr[i]);
		for(int j=1;j<=lr[i];j++)
			scanf("%d",&row[i][j]);
		dlr[i]=0;
	}
	for(int i=1;i<=c;i++){
		scanf("%d",&lc[i]);
		for(int j=1;j<=lc[i];j++)
			scanf("%d",&col[i][j]);
		dlc[i]=0;
	}
	dfs(1,1);
	printf("%d",ans);
	return 0;
}
/*
20 20
0
0
0
2 4 3
4 1 1 3 2
3 2 1 1
3 1 2 1
4 2 2 3 1
1 10
1 8
2 2 4
2 1 1
4 1 1 1 1
5 1 1 1 1 1
3 1 4 1
3 1 2 1
2 1 1
1 2
0
0

0
0
0
0
1 4
4 1 1 2 4
3 2 3 1
4 1 3 1 1
5 1 3 1 1 1
4 1 2 2 1
4 2 2 3 1
4 1 4 1 1
4 1 5 2 1
3 1 5 1
3 2 1 4
1 4
0
0
0
0



*/

