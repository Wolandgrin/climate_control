//#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <EDB.h>
#define TABLE_SIZE 512
#define TABLE_NAME "p.db"

EDB  edb_proc(&writer_proc, &reader_proc);

struct Struct_proc {
  int id_proc;
  char proc[15];
} struct_proc;

void init_table(){
  char * f = TABLE_NAME;
  File file_proc;
  if (SD.exists(f)){
    file_proc = SD.open(f, FILE_WRITE);
    edb_proc.open(0);
  } else {
    file_proc = SD.open(f, FILE_WRITE);
    edb_proc.create(0, TABLE_SIZE, sizeof(struct_proc));
  }
  file_proc.close();
}

void writer_proc (unsigned long address, byte data) {
  File file_proc;
  file_proc = SD.open(TABLE_NAME, FILE_WRITE);
  file_proc.seek(address); 
  file_proc.write(data); 
  file_proc.flush();
  file_proc.close();
}

byte reader_proc (unsigned long address) { 
  File file_proc;
  file_proc = SD.open(TABLE_NAME, FILE_WRITE);
  file_proc.seek(address); 
  byte b = file_proc.read();   
  file_proc.close();
  return b;
}

void edb_insert_proc(int id_proc, char * proc) {
  struct_proc.id_proc = id_proc; 
  strcpy( struct_proc.proc, proc );
  edb_proc.appendRec(EDB_REC struct_proc);
}

//void edb_delete_proc(int id){
//  for (int recno=1; recno <= edb_proc.count(); recno++) {
//    edb_proc.readRec(recno, EDB_REC struct_proc);
//    if (struct_proc.id_proc == id) {
//      edb_proc.deleteRec(recno);
//      break;
//    }
//  }
//}

//void edb_update_proc(int id_proc, char * proc){ 
//  struct_proc.id_proc   = id_proc; 
//  strcpy( struct_proc.proc, proc );
//  edb_proc.updateRec(id_proc, EDB_REC struct_proc);
//}

void edb_read_proc(unsigned long recno_proc){
  edb_proc.readRec(recno_proc, EDB_REC struct_proc);
}

int edb_max_proc_id() {
  int max=0;
  Struct_proc s;
  for (int recno=1; recno <= edb_proc.count(); recno++) {
    edb_proc.readRec(recno, EDB_REC s);
    
    if (s.id_proc > max) { max = s.id_proc; }
  }
  return max;
}

boolean iniciar_sd_card() {
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  if (!SD.begin(4)) { return false; }
  return true;
}

boolean login(char * linebuf) {
  char usuario_senha[] = "admin:admin";
  int t = strlen(usuario_senha);
  
  int tamanhoEnc = (((t-1) / 3) + 1) * 4;   //tamanho da string codificada
  char out[tamanhoEnc];
  base64_encode(out, usuario_senha, t+1 );
  
  //---desconta é usado pra eliminar os caracteres '='
  int desconta = 0;
  if ((t%3) == 1) { desconta = 2; }
  if ((t%3) == 2) { desconta = 1; }
  
  char out2[tamanhoEnc-desconta];
  
  byte i;
  for (i=0; i<(tamanhoEnc-desconta);i++){ out2[i] = out[i]; }
  out2[i] = '\0';
  
  return ( strstr(linebuf, out2)>0 );
}

EthernetServer * server;

void iniciar_ethernet(){
  byte ip[4]      = {192,168,200,188};
  //byte gateway[4] = {192,168,200,254};
  //byte subnet[4]  = {255,255,255,0};
  byte mac[6]     = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
  int  porta      = 80;

  server = new EthernetServer(porta);
  Ethernet.begin(mac);
  server->begin();
}

void write_from_file(EthernetClient &client, char * file_name){
  File webFile = SD.open(file_name);
  if (webFile) {
    while(webFile.available()) {
      client.write(webFile.read()); 
    }
    webFile.close();
  }
}

void html_cab_200_ok(EthernetClient &client){
  client.println(F("HTTP/1.1 200 OK\n"
                   "Content-Type: text/html\n"
                   "Connection: keep-alive\n\n"));
}

void html_logoff(EthernetClient &client){
  write_from_file(client, "logoff.htm");
}

void html_autenticar(EthernetClient &client) {
  write_from_file(client, "aut.htm");
}

void html_proc_list(EthernetClient &client) {
  html_cab_200_ok(client);
  write_from_file(client, "plist.htm");
}

void js_proc_list(EthernetClient &client){
  //cabecalho para javascript
  client.println(F("HTTP/1.1 200 OK\n"
                   "Content-Type: text/javascript\n"
                   "Connection: keep-alive\n\n"));
                              
  client.print("var json_proc=\'{\"processos\":[");  
  char virgula = ' ';
  for (int recno = 1; recno <= edb_proc.count(); recno++) {
    edb_proc.readRec(recno, EDB_REC struct_proc);
    
    client.print(virgula);
    client.print("{\"id_proc\":\"");
    client.print(struct_proc.id_proc);
    client.print("\", \"proc\":\"");
    client.print(struct_proc.proc);
    client.print("\"}");
    
    virgula = ',';
  }  
  
  client.print("]}\';");  
}

//void delete_proc(char * linebuf) {  
//  char * p = strstr(linebuf, "p=");  // /GET ?p=123
//  char r[5] = {'\0'};
//  
//  if (p) {
//    byte i=0;
//    while (p[i+2] >= '0' && p[i+2] <= '9') {
//      r[i] = p[i+2];
//      i++;
//      if (i==4) {break;}
//    }
//    r[i]='\0';
//  }
//
//  if (r[0] >= 48 && r[0] <= 57) {  //48->0  57->9
//    edb_delete_proc(atoi(r));  //apaga o registro
//  }  
//}

void insert_proc(char * linebuf) {  
  char * p = strstr(linebuf, "p=");  // /GET ?p=123
  char r[16] = {'\0'};
  
  if (p) {
    byte i=0;
    while (p[i+2] != ' ') {
      r[i] = (p[i+2] == '+') ? r[i] = ' ' : r[i] = p[i+2];
      i++;
      if (i==15) {break;}
    }
    r[i]='\0';
  }

  edb_insert_proc(edb_max_proc_id()+1, r);  //inclui registro
}

void exec_ethernet(){
  EthernetClient client = server->available();
  if (client) {
    char linebuf[80];
    memset(linebuf, 0, sizeof(linebuf));
    
    int     charCount          = 0;
    boolean auth        = false;
    boolean currentLineIsBlank = true;
    boolean logoff             = false;
    boolean jsProcList         = false;
    boolean listProc           = false;
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        
        linebuf[charCount] = c;
        
        if (charCount<sizeof(linebuf)-1) { charCount++; }
        Serial.write(c);
        
        if (c == '\n' && currentLineIsBlank) {
          if (auth && !logoff ) {
            if(jsProcList) { js_proc_list(client);   } 
            if(listProc)   { html_proc_list(client); }
          } else {
            logoff ? html_logoff(client) : html_autenticar(client);
          }
          break;
        }
        if (c == '\n') { 
          currentLineIsBlank = true;               
          
          if (strstr(linebuf, "Authorization: Basic")>0 ) { if ( login(linebuf) )   {  auth = true; } }
          if (strstr(linebuf, "GET /logoff"         )>0 ) { logoff      = true; }
          if (strstr(linebuf, "GET / "              )>0 ) { listProc    = true; }
          if (strstr(linebuf, "GET /js_proc_list"   )>0 ) { jsProcList  = true; }
//          if (strstr(linebuf, "GET /del_proc"       )>0 ) { listProc    = true; delete_proc(linebuf); }
          if (strstr(linebuf, "GET /insert_proc"    )>0 ) { listProc    = true; insert_proc(linebuf); }
          
          memset(linebuf, 0, sizeof(linebuf));
          charCount = 0;
        } else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    
    delay(1);
    client.stop();
  }
}

void a3_to_a4(unsigned char * a4, unsigned char * a3);
void a4_to_a3(unsigned char * a3, unsigned char * a4);
unsigned char b64_lookup(char c);

int base64_encode(char *output, char *input, int inputLen) {
  const char b64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int i=0, j=0, encLen=0;
  unsigned char a3[3], a4[4];

  while(inputLen--) {
    a3[i++] = *(input++);
    if(i == 3) {
      a3_to_a4(a4, a3);
      for(i=0; i<4; i++) { output[encLen++] = b64_alphabet[a4[i]]; }
      i = 0;
    }
  }

  if (i) {
    for(j = i; j < 3; j++)     { a3[j] = '\0';                           }
    a3_to_a4(a4, a3);
    for(j = 0; j < i + 1; j++) { output[encLen++] = b64_alphabet[a4[j]]; }
    while((i++ < 3))           { output[encLen++] = '=';                 }
  }
  output[encLen] = '\0';
  return encLen;
}

void a3_to_a4(unsigned char * a4, unsigned char * a3) {
  a4[0] = (a3[0]  & 0xfc) >> 2;
  a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
  a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
  a4[3] = (a3[2] & 0x3f);
}

unsigned char b64_lookup(char c) {
  if(c >='A' && c <='Z') return c - 'A';
  if(c >='a' && c <='z') return c - 71;
  if(c >='0' && c <='9') return c + 4;
  if(c == '+') return 62;
  if(c == '/') return 63;
  return -1;
}
void setup() {
  Serial.begin(9600);
  iniciar_sd_card();
  iniciar_banco_dados();
  iniciar_ethernet();
}

void loop() {
  exec_ethernet();
}
