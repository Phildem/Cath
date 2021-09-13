/*
Cath Copyright Cyrob 2021
Cyrob Arduino Task helper by Philippe Demerliac

See my presentation video in French : https://youtu.be/aGwHYCcQ3Io
See also for v1.3 : https://youtu.be/ph57EpJPs5E

=====================================================================================
==========================   OPEN SOURCE LICENCE   ==================================
=====================================================================================

Copyright 2021 Philippe Demerliac Cyrob.org

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.

IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

................................................................................................................
Release history
................................................................................................................
Version Date        Author    Comment
1.0     30/08/2021  Phildem   First version tested ok
1.1     05/09/2021  Phildem   Misc fixes, better comments and presentation
1.2     06/09/2021  Phildem   Remove unnecessary Cath:: in Cath class definition, (Warning removed)
1.3     08/09/2021  Phildem   Misc comments/name fixes, Memory optimised, __CathOpt_SmallCounter__ option added
1.4     13/09/2021  Soif      Fixes english comments & indentation
*/


//____________________________________________________________________________________________________________
// Start of Cath definition__________________________________________________________________________________
 
#define kMaxCathTask    9         // Max Number of task instances. MUST BE >= to tasks instancied

#define __CathOpt_SmallCounter__  // Comment this line to allow 32 bit delay. If not, max period is 65536 ms

#ifdef __CathOpt_SmallCounter__
typedef uint16_t CathCnt;
#else
typedef uint32_t CathCnt;
#endif


class Cath{

  public:

// Derived class MUST implement these 2 methods
  virtual void          SetUp() =0;                 // Called at setup
  virtual void          Loop()  =0;                 // Called periodically

  CathCnt               m_CurCounter;               // Curent number of ms before next Loop call
  CathCnt               m_LoopDelay;                // Default period of Loop call (in ms)

  static uint8_t        S_NbTask;                   // Actual number of task instances
  static Cath*          S_CathTasks[kMaxCathTask];  // Array of task object pointers
  static uint8_t        S_LastMilli;                // Used to call every ms (a byte is enought to detect change)

  //..............................................................
  // Must be called in task constructors to register in the task list
  // WARNING : think to set kMaxCathTask as needed
  // Task   : Pointer to the derivated task to register
  // Period : Loop call Period (in ms). WARNING do not pass 0!
  // Offset : Delay of the first call in ms (1 def). WARNING do not pass 0!
  static void S_Register(Cath* Task,CathCnt Period,CathCnt Offset=1){
    Task->m_LoopDelay=Period;
    Task->m_CurCounter= Offset;
    Cath::S_CathTasks[Cath::S_NbTask++]=Task;
  }

  //..............................................................
  // Must be called once in Arduino setup to call all the task setups
  static void S_SetUp(){
    for(int T=0;T<S_NbTask;T++)
      Cath::S_CathTasks[T]->SetUp();
  }

   //..............................................................
  // Must be called once in Arduino Loop to call all the task loop if needed
  static void S_Loop(){
    uint8_t CurMilli=millis();
    if (CurMilli!=S_LastMilli) {
      S_LastMilli=CurMilli;
      for(int T=0;T<S_NbTask;T++) 
        if ( Cath::S_CathTasks[T]->m_CurCounter--==0) {
          Cath::S_CathTasks[T]->m_CurCounter=Cath::S_CathTasks[T]->m_LoopDelay;
          Cath::S_CathTasks[T]->Loop();
        }
     }
  }

};

//Cath static variables definitions 
//(Note set to 0 for code clarity but done by default anyway because they are static)
uint8_t       Cath::S_NbTask=0;
Cath*         Cath::S_CathTasks[kMaxCathTask];
uint8_t       Cath::S_LastMilli=0;

// End of Cath definition ___________________________________________________________________________________
//___________________________________________________________________________________________________________




//****************************************************************************************************************
// I/O Abstraction

#define kOutPinSlowBlink  4     // Output pins where different leds are connected
#define kOutPinFastBlink  5     
#define kOutPinAssyBlink  6
#define kOutPinAorB       7
#define kOutPinAandB      8
#define kOutPinAxorB      9

#define kInPinA           2     // Input pins where pushbuttons are connected (Internal pullup required)
#define kInPinB           3


//****************************************************************************************************************
// Globals

bool gPushA=false;      // Memory state of button A, true if pushed, debounced
bool gPushB=false;      // Memory state of button B, true if pushed, debounced

//Exemple task ...........................................................................................

//Blinker ..........................................................................................
class Blinker: public Cath{

  public:

  Blinker(uint8_t Pin,unsigned long Period,unsigned long Offset=1){
    m_Pin=Pin;
    Cath::S_Register(this,Period,Offset);
  }

  void SetUp(){
    pinMode(m_Pin,OUTPUT);
    digitalWrite(m_Pin, LOW);
  }

  void Loop(){
    digitalWrite(m_Pin,!digitalRead(m_Pin));
  }

  uint8_t     m_Pin;    // Number id of the output pin to blink
};

//Assy Blinker ..........................................................................................
class ABlinker: public Cath{

  public:

  ABlinker(){
    Cath::S_Register(this,2000,666);
  }

  void SetUp(){
    pinMode(kOutPinAssyBlink,OUTPUT);
    digitalWrite(kOutPinAssyBlink, LOW);
    m_State=0;
  }

  void Loop(){

    if (m_State++ & 1){
       digitalWrite(kOutPinAssyBlink,HIGH);
       m_CurCounter=50;                       // Here we force the next call to a shorter value
    }
    else {
      digitalWrite(kOutPinAssyBlink,LOW);
    }
  }

  unsigned char m_State;    // State counter, short pulse if Odd
};

//PushBut ..........................................................................................
class PushBut: public Cath{

  public:

  PushBut(uint8_t Pin, bool* Store){
    m_Pin=Pin;
    m_Store=Store;
    Cath::S_Register(this,20);    // Here 20ms is for switch deboudcing
  }

  void SetUp(){
    pinMode(m_Pin,INPUT_PULLUP);
  }

  void Loop(){
    *m_Store=!digitalRead(m_Pin); // We invert the value because the switch in inverted (connected to ground)
  }

    uint8_t     m_Pin;      // Number Id of input Pin to test
    bool*       m_Store;    // Boolean value to store the result
};

//PinAorB ..........................................................................................
class PinAorB: public Cath{

  public:

  PinAorB(){
    Cath::S_Register(this,20);
  }

  void SetUp(){
    pinMode(kOutPinAorB,OUTPUT);
  }

  void Loop(){
    digitalWrite(kOutPinAorB, gPushA || gPushB);
  }
};

//PinAandB ..........................................................................................
class PinAandB: public Cath{

  public:

  PinAandB(){
    Cath::S_Register(this,20);
  }

  void SetUp(){
    pinMode(kOutPinAandB,OUTPUT);
  }

  void Loop(){
    digitalWrite(kOutPinAandB, gPushA && gPushB);
  }
};

//PinAxorB ..........................................................................................
class PinAxorB: public Cath{

  public:

  PinAxorB(){
    Cath::S_Register(this,60);
  }

  void SetUp(){
    pinMode(kOutPinAxorB,OUTPUT);
  }

  void Loop(){
    digitalWrite(kOutPinAxorB, !gPushA != !gPushB && !digitalRead(kOutPinAxorB));
  }
};

//****************************************************************************************************************
// Global tasks instanciation

// 3 Instances of the blinker task
Blinker   BuiltIn(LED_BUILTIN,100,50);
Blinker   Blinker1(kOutPinSlowBlink,500);
Blinker   Blinker2(kOutPinFastBlink,1500,500);

// 2 Instances of the PushBut task
PushBut   PushA(kInPinA,&gPushA);
PushBut   PushB(kInPinB,&gPushB);

// 1 instance by tasks for these one
ABlinker  Assy;
PinAorB   AorB;
PinAandB  AandB;
PinAxorB  AxorB;


//-----------------------------------------------------------------------------------------------------------------
void setup() {
  Cath::S_SetUp();    // Just ask Cath to call the task's setup
}

//-----------------------------------------------------------------------------------------------------------------
void loop() {
  Cath::S_Loop();    // Just ask Cath to call the task's loop
}

// That's all folks.....
