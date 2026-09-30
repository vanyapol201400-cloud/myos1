#include <stdint.h>
extern void vga_put_char_at(int,int,char,uint8_t);
extern int keyboard_has_char(void);
extern char keyboard_get_char(void);
static int b[9];
static int W(void){int L[8][3]={{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};
for(int i=0;i<8;i++){int a=L[i][0],c=L[i][1],d=L[i][2];if(b[a]&&b[a]==b[c]&&b[c]==b[d])return b[a];}
for(int i=0;i<9;i++)if(!b[i])return 0;return 3;}
static void D(int cur,int w){
for(int i=0;i<80*25;i++)vga_put_char_at(i%80,i/80,' ',0x07);
const char*t="TIC-TAC-TOE";for(int i=0;t[i];i++)vga_put_char_at(32+i,1,t[i],0x1F);
const char*h="WASD/Enter/Esc";for(int i=0;h[i];i++)vga_put_char_at(28+i,23,h[i],0x07);
for(int i=0;i<15;i++){vga_put_char_at(30,4+i,'|',0x0A);vga_put_char_at(37,4+i,'|',0x0A);}
for(int i=25;i<43;i++){vga_put_char_at(i,9,'-',0x0A);vga_put_char_at(i,14,'-',0x0A);}
for(int i=0;i<9;i++){int r=i/3,c=i%3,x=27+c*7,y=6+r*5;
char ch=b[i]==1?'X':b[i]==2?'O':'.';
uint8_t a=(i==cur&&!w)?0x70:(b[i]==1?0x0C:b[i]==2?0x0B:0x08);
vga_put_char_at(x,y,ch,a);}
if(w==1){const char*s="*** YOU WIN ***";for(int i=0;s[i];i++)vga_put_char_at(30+i,17,s[i],0x2F);}
if(w==2){const char*s="*** YOU LOSE ***";for(int i=0;s[i];i++)vga_put_char_at(30+i,17,s[i],0x4F);}
if(w==3){const char*s="*** DRAW ***";for(int i=0;s[i];i++)vga_put_char_at(32+i,17,s[i],0x6F);}}
static void C(void){for(int i=0;i<9;i++)if(!b[i]){b[i]=2;if(W()==2)return;b[i]=0;}
for(int i=0;i<9;i++)if(!b[i]){b[i]=1;if(W()==1){b[i]=2;return;}b[i]=0;}
if(!b[4]){b[4]=2;return;}int co[4]={0,2,6,8};
for(int i=0;i<4;i++)if(!b[co[i]]){b[co[i]]=2;return;}
for(int i=0;i<9;i++)if(!b[i]){b[i]=2;return;}}
void gui_run_tictactoe(void){
for(int i=0;i<9;i++)b[i]=0;
int cur=4,w=0,turn=1;D(cur,w);
while(!w){if(turn){if(keyboard_has_char()){char ch=keyboard_get_char();
if(ch==0x1B)return;
if((ch==0x15||ch=='w'||ch=='W')&&cur>2)cur-=3;
else if((ch==0x17||ch=='s'||ch=='S')&&cur<6)cur+=3;
else if((ch==0x14||ch=='a'||ch=='A')&&cur%3>0)cur--;
else if((ch==0x16||ch=='d'||ch=='D')&&cur%3<2)cur++;
else if((ch=='\n'||ch==' ')&&!b[cur]){b[cur]=1;turn=0;}D(cur,w);}}
else{C();turn=1;D(cur,w);}w=W();}
D(cur,w);while(!keyboard_has_char()){__asm__ volatile("hlt");}keyboard_get_char();}
