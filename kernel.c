
/*
  Header Declarations
*/

#define DIR_SECTOR 2
#define MAX_PROG_SIZE 13312
#define SECTOR_SIZE 512
#define MAX_FILENAME_LENGTH 6
#define TRUE 1
#define processes 8

/*
  Milestone 1
*/
void printString(char *);
void readString(char *);
void printChar(char);
void readSector(char *, int);
void writeSector(char *, int);
int div(int, int);
int mod(int, int);
void handleInterrupt21(int *, int *, int *, int, int);

/*
  Milestone 2
*/

int readFile(char *, char *);
void executeProgram(char *, int);
void terminate();

/*
  Milestone 2 Testing
*/

void task1();
void task2();
void task3();
void runShell();

/*
  Milestone 3
*/

void deleteFile(char *);
void filename_cpy(char *, char *);
void printVariable(int);
void find_file(int *, int *, int *, char *, char *);
int writeFile(char *, char *, int);

/*
  Milestone 3 Testing
*/

void task1_3();
void task2_3();
void task3_3();

/*
  Milestone 4
*/

/*
  Process table to keep track of processes activity up till now
  First dimension is the process active or not. 1 is active
  Second dimesion is the process stack pointer
*/

int currentProcess, calls;
int processTable[processes][2];

void handleTimerInterrupt(int , int);
void executeProgram2(char *);
void terminate2();
void kill(int);
void runShell2();


int main() {
  int i;
  for(i=0;i<processes;i++) {
    processTable[i][0] = 0;
    processTable[i][1] = 0xFF00;
  }
  currentProcess = -1;
  calls = 95;

  makeInterrupt21();
  makeTimerInterrupt();

  // Step 4.a testing

  // interrupt(0x21, 4, "hello1\0", 0, 0);
  // interrupt(0x21, 4, "hello2\0", 0, 0);

  // Step 4.b testing

  // runShell2();
  // interrupt(0x21, 4, "phello\0", 0, 0); 

  // Step 5 testing
  runShell2();

  while (TRUE)
    ;
  return 0;
}

/*
  Milestone 4 Tasks
*/

/*
  Step 1
*/

// void handleTimerInterrupt(int segment, int sp) {
//   printString("Tic\0");
//   returnFromTimer(segment, sp);
// }

/*
  Step 2, 3 & 4
*/

void handleTimerInterrupt(int segment, int sp) {
  int nextProcess, seg, stack_pointer;
  int cnt = processes;

  setKernelDataSegment();

  if(calls == 100) {
    calls = 0;
    nextProcess = mod(currentProcess + 1, processes);
    while(cnt > 0 && !processTable[nextProcess][0]) {
      nextProcess = mod(nextProcess + 1, processes);
      if(currentProcess == -1 && nextProcess == 0) {
        nextProcess = -1;
        break;
      }

    }
    
    if(nextProcess == currentProcess) {
      restoreDataSegment();
      returnFromTimer(segment, sp);
    } else {
      processTable[currentProcess][1] = sp;
      currentProcess = nextProcess;
      seg = (currentProcess + 2) * 0x1000;
      stack_pointer = processTable[currentProcess][1];
      restoreDataSegment();
      returnFromTimer(seg, stack_pointer);
    }

  } else {
    calls++;
    restoreDataSegment();
    returnFromTimer(segment, sp);
  }

  
}

void executeProgram2(char *name) {
  char buffer[MAX_PROG_SIZE];
  int i, file_found, freeIDX, segment;

  for(freeIDX = 0; freeIDX < processes; freeIDX++) {
    setKernelDataSegment();
    if(!processTable[freeIDX][0]) {
      restoreDataSegment();
      break;
    }
    restoreDataSegment();
  }
    
  if(freeIDX == processes)
    return;

  
  segment = (freeIDX + 2) * 0x1000;
  file_found = readFile(name, buffer);

  if (!file_found)
    return;

  for (i = 0; i < MAX_PROG_SIZE; i++) {
    putInMemory(segment, i, buffer[i]);
  }

  initializeProgram(segment);

  setKernelDataSegment();
  processTable[freeIDX][0] = 1;
  restoreDataSegment();

}

void terminate2() {
  setKernelDataSegment();
  processTable[currentProcess][0] = 0;
  while(TRUE);
}

/*
  Step 5
*/

void kill(int process) {
  setKernelDataSegment();
  processTable[process][0] = 0;
  processTable[process][1] = 0xFF00;
  restoreDataSegment();
}

void runShell2() { 
  interrupt(0x21, 4, "shell\0", 0, 0);
}

/*
  Milestone 1 Tasks
*/

void printString(char *chars) {
  int i = 0;
  while (chars[i] != '\0') {
    printChar(chars[i]);
    i++;
  }
}

void printChar(char c) { interrupt(0x10, 0xE * 256 + c, 0, 0, 0); }

void readString(char *chars) {
  char c;
  int idx = 0;

  while (1) {
    c = interrupt(0x16, 0, 0, 0, 0);

    if (c == 0xd) { // Enter key pressed

      printChar('\n');
      printChar('\r');

      // EOL
      chars[idx] = '\n';
      chars[idx + 1] = '\r';
      chars[idx + 2] = 0x0;
      break;
    }

    if (c == 0x8 && idx > 0) { // Backspace pressed
      printChar(c);
      printChar('\0');
      chars[--idx] = '\0';
    } else {
      chars[idx++] = c;
    }
    printChar(c);
  }
}

void readSector(char *buffer, int sector) {
  int rel_sector = mod(sector, 18) + 1;
  int head = mod(div(sector, 18), 2);
  int track = div(sector, 36);
  interrupt(0x13, 2 * 256 + 1, buffer, track * 256 + rel_sector,
            head * 256 + 0);
}

void writeSector(char *buffer, int sector) {
  int rel_sector = mod(sector, 18) + 1;
  int head = mod(div(sector, 18), 2);
  int track = div(sector, 36);
  interrupt(0x13, 3 * 256 + 1, buffer, track * 256 + rel_sector,
            head * 256 + 0);
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

void handleInterrupt21(int ax, int bx, int cx, int dx) {
  if (ax == 0) {
    printString(bx);
  } else if (ax == 1) {
    readString(bx);
  } else if (ax == 2) {
    readSector(bx, cx);
  } else if (ax == 3) {
    readFile(bx, cx);
  } else if (ax == 4) {
    executeProgram2(bx);
  } else if (ax == 5) {
    terminate2();
  } else if (ax == 6) {
    writeSector(bx, cx);
  } else if (ax == 7) {
    deleteFile(bx);
  } else if (ax == 8) {
    writeFile(bx, cx, dx);
  } else if(ax == 9) {
    kill(bx);
  } else if (ax == 24) {
    printVariable(bx);
  } else {
    printString("Error!!");
  }
}

int readFile(char *filename, char *buffer) {
  char dir[SECTOR_SIZE];
  char tmp_filename[MAX_FILENAME_LENGTH];
  int i, j;
  int file_found;

  readSector(dir, 2);

  filename_cpy(tmp_filename, filename);
  find_file(&i, &j, &file_found, dir, tmp_filename);

  if (file_found) {
    for (j = i + MAX_FILENAME_LENGTH; j < i + 32; j++) {
      if (dir[j] == '\0')
        break;
      readSector(buffer + SECTOR_SIZE * (j - i - 6), dir[j]);
    }
  }

  return file_found;
}

void deleteFile(char *filename) {

  char map[SECTOR_SIZE];
  char dir[SECTOR_SIZE];
  char tmp_filename[MAX_FILENAME_LENGTH];
  int i, j, file_found;
  char* message;

  readSector(map, 1);
  readSector(dir, 2);

  filename_cpy(tmp_filename, filename);
  find_file(&i, &j, &file_found, dir, tmp_filename);

  if (file_found) {
    dir[i] = 0x00;
    for (j = i + MAX_FILENAME_LENGTH; j < i + 32; j++) {
      if (dir[j] == '\0')
        break;
      map[dir[j]] = 0x00;
    }

    writeSector(map, 1);
    writeSector(dir, 2);
    
  } else {
    filename[0] = '\0'; // file not found
  }
}

int writeFile(char *filename, char *buffer, int sec_num) {

  char map[SECTOR_SIZE];
  char dir[SECTOR_SIZE];
  char tmp_filename[MAX_FILENAME_LENGTH];
  int free_sectors[SECTOR_SIZE];
  int i, j, file_found, cnt;

  readSector(map, 1);
  readSector(dir, 2);

  filename_cpy(tmp_filename, filename);

  find_file(&i, &j, &file_found, dir, tmp_filename);

  if(!file_found){
    /*
      Find free sectors
    */

    cnt = 0;
    for (i = 0; i < SECTOR_SIZE; i++) {
      if (map[i] == 0x00) {
        free_sectors[cnt++] = i;
      }
    }

    /*
      Check if we can write the file to the disc
    */

    for (i = 0; i < SECTOR_SIZE; i += 32) {
      if (dir[i] == 0x00)
        break;
    }

    if (cnt < sec_num || i >= SECTOR_SIZE) {
      return 0;
    }

    /*
      Copy filename to sector
    */

    for (j = i; j < i + MAX_FILENAME_LENGTH; j++) {
      dir[j] = tmp_filename[j - i];
    }

    i = i + MAX_FILENAME_LENGTH; // start of sectors

    /*
      Write the file to the free sectors
    */

    for (; j < i + 26; j++) {
      dir[j] = 0x00;
    }

    for (j = 0; j < sec_num; j++) {
      map[free_sectors[j]] = 0xFF;
      dir[i + j] = free_sectors[j];
      writeSector(buffer + SECTOR_SIZE * j, free_sectors[j]);
    }

    writeSector(map, 1);
    writeSector(dir, 2);
  }else{
    // Because C is very predictible :)
    printChar('F');
    printChar('i');
    printChar('l');
    printChar('e');
    printChar(' ');
    printChar('n');
    printChar('a');
    printChar('m');
    printChar('e');
    printChar(' ');
    printChar('a');
    printChar('l');
    printChar('r');
    printChar('e');
    printChar('a');
    printChar('d');
    printChar('y');
    printChar(' ');
    printChar('e');
    printChar('x');
    printChar('i');
    printChar('s');
    printChar('t');
    printChar('s');
    printChar('\n');
    printChar('\r');

  }
}

void executeProgram(char *name, int segment) {
  char buffer[MAX_PROG_SIZE];
  int i, file_found;

  file_found = readFile(name, buffer);

  if (!file_found)
    return;

  for (i = 0; i < MAX_PROG_SIZE; i++) {
    putInMemory(segment, i, buffer[i]);
  }

  launchProgram(segment);
}

void terminate() {

  char shell[MAX_FILENAME_LENGTH];
  shell[0] = 's';
  shell[1] = 'h';
  shell[2] = 'e';
  shell[3] = 'l';
  shell[4] = 'l';
  shell[5] = 0x0;

  interrupt(0x21, 4, shell, 0x2000, 0);
}

void find_file(int *i, int *j, int *file_found, char *dir, char *tmp_filename) {
  *i = 0;
  for (*i = 0; *i < SECTOR_SIZE; *i += 32) {
    *file_found = 1;
    for (*j = *i; *j < *i + MAX_FILENAME_LENGTH; (*j)++) {
      if (dir[*j] != tmp_filename[*j - *i]) {
        *file_found = 0;
        break;
      }
    }
    if (*file_found)
      break;
  }
}

void filename_cpy(char *tmp_filename, char *filename) {
  int i;

  for (i = 0; i < MAX_FILENAME_LENGTH; i++) {
    if (filename[i] == '\0')
      break;
    tmp_filename[i] = filename[i];
  }

  for (; i < MAX_FILENAME_LENGTH; i++)
    tmp_filename[i] = 0x0;
}

void printVariable(int i) {

  char buf[SECTOR_SIZE];
  int j, k, tmp, m;

  tmp = i;
  k = 0;

  if (i == 0) {
    printChar('0');
    printChar('\r');
    printChar('\n');
    return;
  }

  for (j = 0; j < SECTOR_SIZE; j++)
    buf[j] = 0x0;

  while (tmp > 0) {
    tmp = div(tmp, 10);
    k++;
  }

  buf[k] = '\r';
  buf[k + 1] = '\n';
  k--;

  while (i > 0) {
    j = mod(i, 10);
    i = div(i, 10);
    buf[k] = '0' + j;
    k--;
  }
  printString(buf);
}

/*
  Tasks testing
*/

void task1() {
  char buffer[MAX_PROG_SIZE]; /*this is the maximum size of a file*/
  interrupt(0x21, 3, "abc.txt", buffer, 0); /*read the file into buffer*/
  interrupt(0x21, 0, buffer, 0, 0);         /*print out the file*/
}

void task2() { interrupt(0x21, 4, "tstprg\0", 0x2000, 0); }

void task3() { interrupt(0x21, 4, "tstpr2\0", 0x2000, 0); }

void task1_3() {
  char buf[512];
  writeSector("Hello World", 40);
  readSector(buf, 40);
  printString(buf);
}

void task2_3() {
  char buffer[13312];
  makeInterrupt21();
  interrupt(0x21, 7, "messag\0", 0, 0);      // delete messag
  interrupt(0x21, 3, "messag\0", buffer, 0); // try to read messag
  interrupt(0x21, 0, buffer, 0, 0);          // print out the contents of buffer
}

void task3_3() {
  int i = 0;
  char buffer1[13312];
  char buffer2[13312];
  buffer2[0] = 'h';
  buffer2[1] = 'e';
  buffer2[2] = 'l';
  buffer2[3] = 'l';
  buffer2[4] = 'o';
  for (i = 5; i < 13312; i++)
    buffer2[i] = 0x0;
  makeInterrupt21();
  interrupt(0x21, 8, "testW\0", buffer2, 1); // write file testW
  interrupt(0x21, 3, "testW\0", buffer1, 0); // read file testW
  interrupt(0x21, 0, buffer1, 0, 0);         // print out  contents of testW
}

void runShell() { interrupt(0x21, 4, "shell\0", 0x2000, 0); }