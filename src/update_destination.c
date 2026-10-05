#include "app.h"
int update_destination_id(const char *id){
 if(!strcmp(id,"internal"))return 0;
 if(!strcmp(id,"usb0"))return 1;
 if(!strcmp(id,"usb1"))return 2;
 return -1;
}
const char *update_destination_root(int id){
 return id==0?app.root:id==1?"/mnt/usb0":id==2?"/mnt/usb1":NULL;
}
int update_destination_available(int id){
 const char *root=update_destination_root(id);if(!root)return 0;
 if(id==0)return 1;
#ifdef _WIN32
 /* USB destinations here belong to a PS4, not to the Windows test host. */
 return 0;
#else
 struct stat st;return !stat(root,&st)&&S_ISDIR(st.st_mode);
#endif
}
int update_destination_prepare(int id,char *error,size_t cap){
 const char *root=update_destination_root(id);
 if(!root){snprintf(error,cap,"Destino de atualizacao invalido.");return -1;}
 if(id&&!update_destination_available(id)){snprintf(error,cap,"A pen USB selecionada nao esta disponivel. Liga uma pen exFAT ou FAT32 e tenta novamente.");return -1;}
 if(!id)make_dir(root);
 return 0;
}
