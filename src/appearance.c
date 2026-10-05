#include "appearance.h"
#include "platform.h"
int accent_parse(const char *text,size_t size,uint32_t *color){
 if(size!=7||text[0]!='#')return -1;uint32_t value=0;
 for(size_t i=1;i<7;i++){unsigned c=(unsigned char)text[i],n;
  if(c>='0'&&c<='9')n=c-'0';else if(c>='a'&&c<='f')n=c-'a'+10;else if(c>='A'&&c<='F')n=c-'A'+10;else return -1;
  value=(value<<4)|n;
 }*color=value;return 0;
}
uint32_t accent_load(const char *root){
 char path[600],text[8];uint32_t color=ACCENT_DEFAULT;snprintf(path,sizeof(path),"%s/appearance.txt",root);
 FILE *f=fopen(path,"rb");if(f){size_t n=fread(text,1,sizeof(text),f);if(!ferror(f))accent_parse(text,n,&color);fclose(f);}return color;
}
int accent_save(const char *root,uint32_t color){
 char path[600],temp[600],text[8];snprintf(path,sizeof(path),"%s/appearance.txt",root);snprintf(temp,sizeof(temp),"%s/appearance.tmp",root);snprintf(text,sizeof(text),"#%06x",(unsigned)(color&0xffffff));
 FILE *f=fopen(temp,"wb");if(!f)return -1;int bad=fwrite(text,1,7,f)!=7;if(fflush(f))bad=1;if(fclose(f))bad=1;
 if(!bad){
#ifdef _WIN32
  bad=!MoveFileExA(temp,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
#else
  bad=rename(temp,path)!=0;
#endif
 }if(bad)remove(temp);return bad?-1:0;
}
