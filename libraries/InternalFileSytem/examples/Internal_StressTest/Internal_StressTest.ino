/*********************************************************************
 This is an example for our nRF52 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

using namespace Adafruit_LittleFS_Namespace;

/* This example perform a stress test on Little FileSystem on internal flash.
 * There are 4 different thread sharing same loop() code with different priority
 *    loop (low), normal, high, highest
 * Each will open and write a file of its name (+ .txt). Task takes turn writing until timeout
 * to print out the summary
 */

// timeout in seconds
#define TIME_OUT      5

bool summarized = false;

// the setup function runs once when you press reset or power the board
void setup() 
{
  Serial.begin(115200);
  while ( !Serial ) yield();   // for nrf52840 with native usb

  Serial.println("Internal Stress Test Example");
  yield();

  // Initialize Internal File System
  InternalFS.begin();

  // Format
  Serial.print("Formatting ... "); Serial.flush();
  InternalFS.format();
  Serial.println("Done"); Serial.flush();

  // Create thread with different priority
  // Although all the thread share loop() code, they are separated threads
  // and running with different priorities

  // Note: default loop() is running at LOW
  Scheduler.startLoop(loop, 1024, TASK_PRIO_NORMAL, "normal");
  Scheduler.startLoop(loop, 1024, TASK_PRIO_HIGH, "high");
  Scheduler.startLoop(loop, 1024, TASK_PRIO_HIGHEST, "highest");
}

void write_files(const char * name)
{
  char fname[20] = { 0 };
  sprintf(fname, "%s.txt", name);

  File file(InternalFS);

  if ( file.open(fname, FILE_O_WRITE) )
  {
    file.println(name);
    file.close();
  }else
  {
    Serial.printf("Failed to open %s\n", fname);
  }
}

void list_files(void)
{
  File dir("/", FILE_O_READ, InternalFS);
  File file(InternalFS);

  while( (file = dir.openNextFile(FILE_O_READ)) )
  {
    if ( file.isDirectory() ) continue;

    Serial.printf("--- %s ---\n", file.name());

    while ( file.available() )
    {
      uint32_t readlen;
      char buffer[64] = { 0 };
      readlen = file.read(buffer, sizeof(buffer));

      Serial.print(buffer);
    }
    file.close();

    Serial.println("---------------\n");
    Serial.flush();
  }
}

// the loop function runs over and over again forever
void loop() 
{
  TaskHandle_t th = xTaskGetCurrentTaskHandle();

  if ( millis() > TIME_OUT*1000 )
  {
    // low priority task print summary
    if ( !summarized && (TASK_PRIO_LOW == uxTaskPriorityGet(th)))
    {
      summarized = true;
      list_files();
    }

    vTaskSuspend(NULL); // suspend task
    return;
  }

  const char* name = pcTaskGetName(th);
  Serial.printf( "Task %s writing ...\n", name );
  Serial.flush();

  // Write files
  write_files(name);

  // lower delay increase chance for high prio task preempt others.
  delay(100);
}