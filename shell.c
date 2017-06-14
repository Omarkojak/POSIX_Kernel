
/*
        Header Definition
*/

#define MAX_PROG_SIZE 13312
#define LINE_LENGTH 80
#define MAX_FILENAME_LENGTH 6
#define MAX_STR_PARTS 10
#define SECTOR_SIZE 512
#define MAX_TOKENS
#define TRUE 1
#define processes 8

void println(char *);
void print(char *);
void printVariable(int);
void getline(char *, char *);
int strlen(char *);
int strcmp(char *, char *);
void resetBuffer(char *, int);
void tokenize(char *, int *);
void readSector(char *, int);
void printChar(char);
int toInteger(char *);

char tokens[MAX_STR_PARTS][LINE_LENGTH];

int main() {
  char line[LINE_LENGTH];
  char buffer[MAX_PROG_SIZE];

  char *cmnds[MAX_STR_PARTS];
  char file_to_load[MAX_FILENAME_LENGTH];

  int cnt;
  int i, j;
  int sector_cnt;
  int file_size;
  int process_to_kill;

  cmnds[0] = "view";
  cmnds[1] = "execute";
  cmnds[2] = "dir";
  cmnds[3] = "create";
  cmnds[4] = "delete";
  cmnds[5] = "copy";
  cmnds[6] = "kill";

  while (TRUE) {
    getline("SHELL>", line);
    tokenize(line, &cnt);

    if (cnt == 0) {
      println("Invalid Command");
      continue;
    }

    if (strcmp(cmnds[0], tokens[0])) { // view command

      interrupt(0x21, 3, tokens[1], buffer);
      if (buffer[0] != '\0') {
        println(buffer);
        resetBuffer(buffer, MAX_PROG_SIZE);
      } else {
        println("File not found.");
      }

    } else if (strcmp(cmnds[1], tokens[0])) { // execute command

      interrupt(0x21, 4, tokens[1], 0x2000, 0);

    } else if (strcmp(cmnds[2], tokens[0])) { // dir command

      readSector(buffer, 2);
      println("");
      print("FILENAME");
      print("          ");
      println("FILE_SIZE(SECTORS)");
      println("===============================================");
      sector_cnt = 0;
      for (i = 0; i < SECTOR_SIZE; i += 32) {
        if (buffer[i] == 0x00)
          continue;
        for (j = i; j < i + MAX_FILENAME_LENGTH; j++) {
          if (buffer[j] != 0x00)
            printChar(buffer[j]);
        }
        for (j = i + MAX_FILENAME_LENGTH; j < i + 32; j++) {
          if (buffer[j] == 0x0)
            break;
        }
        print("                       ");
        printVariable(j - i - MAX_FILENAME_LENGTH);
      }
      print("\r\n");
    } else if (strcmp(cmnds[3], tokens[0])) { // create command
        if(cnt == 2){
          println("Please type your file below:");
          for(i = 0 ; i < MAX_PROG_SIZE;){
              
              interrupt(0x21, 1, line, 0, 0);

              if(line[2] == '\0') break;
              cnt = strlen(line) - 2;
              
              for(j = 0; j < cnt+3 ; j++){
                buffer[j+i] = line[j];
              }
            
              i +=  cnt + 2 ;
              
          }

          sector_cnt = div(strlen(buffer),512);
          if(mod(strlen(buffer),512)) sector_cnt++;

          interrupt(0x21,8,tokens[1],buffer,sector_cnt);
        }else{
          println("Invalid number of arguments.");
        }

    } else if (strcmp(cmnds[4], tokens[0])) { // delete command

      interrupt(0x21, 7, tokens[1], 0);
      if (tokens[1][0] != '\0') {
        println("File deleted.");
        resetBuffer(buffer, MAX_PROG_SIZE);
      } else {
        println("File not found.");
      }

    } else if (strcmp(cmnds[5], tokens[0])) { // copy command
      if(cnt == 3){
        interrupt(0x21, 3, tokens[1], buffer);
        if(buffer[0] != '\0'){
          sector_cnt = div(strlen(buffer) ,512);
          if(mod(strlen(buffer),512)) sector_cnt++;
          interrupt(0x21,8,tokens[2],buffer,sector_cnt);
        }else{
          println("File not found.");
        }
        resetBuffer(buffer);
      }else{
        println("Invalid number of arguments.");
      }
    } else if(strcmp(cmnds[6], tokens[0])){
      process_to_kill = toInteger(tokens[1]);
      if(process_to_kill < 0 || process_to_kill >= processes) {
        println("Invalid process");
      } else {
        interrupt(0x21,9,process_to_kill,0,0);
      }
    } else {
      println("Unknown command.");
    }
  }
  return 0;
}

/*
        Helper Methods
*/

void readSector(char *buffer, int sector) {
  interrupt(0x21, 2, buffer, sector, 0);
}

void printChar(char c) { interrupt(0x10, 0xE * 256 + c, 0, 0, 0); }

void print(char *str) { interrupt(0x21, 0, str, 0, 0); }

void println(char *str) {
  interrupt(0x21, 0, str, 0, 0);
  interrupt(0x21, 0, "\r\n", 0, 0);
}

void printVariable(int x) { interrupt(0x21, 24, x, 0, 0); }

void getline(char *prompt, char *line) {
  print(prompt);
  interrupt(0x21, 1, line, 0, 0);
}

void resetBuffer(char *buffer, int sz) {
  int i;
  for (i = 0; i < sz; i++)
    buffer[i] = 0x00;
}

/*
  Parse Shell Line
*/

void tokenize(char *line, int *cnt) {

  char lastChar;
  int i = 0, j = 0, k = 0, len;

  len = strlen(line) - 2;

  for (i = 0; i < MAX_STR_PARTS; i++)
    resetBuffer(tokens[i], LINE_LENGTH);

  for (i = 0; i < len; i++) {
    if (line[i] == ' ') {
      while (line[i] == ' ')
        i++;
      k = 0;
      j++;
    }
    tokens[j][k] = line[i];
    k++;
  }

  if (len > 0 && line[len - 1] == ' ')
    j--;
  *cnt = j + 1;
}

int strlen(char *line) {
  int i = 0;
  while (line[i] != '\0')
    i++;
  return i;
}

int strcmp(char *a, char *b) {
  int i;
  int lenA = strlen(a);
  int lenB = strlen(b);

  if (lenA != lenB) {
    return 0;
  }

  for (i = 0; i < lenA; i++)
    if (a[i] != b[i])
      return 0;

  return 1;
}

int div(int a, int b) {
  int cnt = 0;
  while (a >= b) {
    a -= b;
    cnt++;
  }
  return cnt;
}

int mod(int a, int b) {
  while (a >= b) {
    a -= b;
  }
  return a;
}

int toInteger(char *s) {
  int len = strlen(s);
  int res = 0, mul = 1, i;

  for(i = len - 1; i >= 0 ; i--) {
    res += (s[i] - '0') * mul;
    mul*=10;
  }
  return res;
}
