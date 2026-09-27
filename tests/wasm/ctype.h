static inline int isalnum(int c){return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9');}
static inline int isxdigit(int c){return (c>='A'&&c<='F')||(c>='a'&&c<='f')||(c>='0'&&c<='9');}
static inline int tolower(int c){return c>='A'&&c<='Z'?c+32:c;}
