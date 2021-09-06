/*
Cath Copyright Cyrob 2021
Cyrob Arduino Task helper by Philippe Demerliac

See my presentation video in French : https://youtu.be/aGwHYCcQ3Io

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
1.1     05/09/2021  Phildem   Misc fix, better comments and presentation
1.2     06/09/2021  Phildem   Remove unnecessary Cath:: in Cath class definition, (Warning)
*/


//____________________________________________________________________________________________________________
// Start of Cath definition __________________________________________________________________________________
 
#define kMaxCathTask    9         // Number max of task instance MUST BE >= at task instancied

class Cath{

  public:

// Derived class MUST implement these 2 methodes
  virtual void          SetUp() =0;                       // Called at setup
  virtual void          Loop()  =0;                       // Called periodically

  unsigned long         m_CurCounter;                     // Curent number of ms before next Loop call
  unsigned long         m_LoopDelay;                      // Default period of Loop call in ms

  static int            S_NbTask;                   // Actual number of task instances
  static Cath*          S_CathTasks[kMaxCathTask];  // Array of task object pointers
  static  unsigned long S_LastMilli;                // Used to call every ms

  //..............................................................
  // Must be called in task constructors to register in the task list
  // WARNING : think to set kMaxCathTask as needed
  // Task :   Ptr on the derivated task to register
  // Period : Period of loop call in ms WARNING do not pass 0!
  // Offset : Demay of first call in ms (1 def) WARNING do not pass 0!
  static void S_Register(Cath* Task,unsigned long Period,unsigned long Offset=1){
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
    unsigned long CurMilli=millis();
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

//Cath static var def
int            Cath::S_NbTask=0;
Cath*          Cath::S_CathTasks[kMaxCathTask];
unsigned long  Cath::S_LastMilli=0;

// End of Cath definition ___________________________________________________________________________________
//___________________________________________________________________________________________________________


//****************************************************************************************************************
// I/O Abstraction

#define kOutPinSlowBlink  4
#define kOutPinFastBlink  5
#define kOutPinAssyBlink  6
#define kOutPinAorB       7
#define kOutPinAandB      8
#define kOutPinAxorB      9
#define kInPinA           2
#define kInPinB           3


//****************************************************************************************************************
// Globals

bool GPushA=false;      // Memory state of button A, true if pushed, debounced
bool GPushB=false;      // Memory state of button A, true if pushed, debounced

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
    digitalWrite(kOutPinAorB, GPushA || GPushB);
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
    digitalWrite(kOutPinAandB, GPushA && GPushB);
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
    digitalWrite(kOutPinAxorB, !GPushA != !GPushB && !digitalRead(kOutPinAxorB));
  }
};

//****************************************************************************************************************
// Global tasks instanciation

// 3 Instances of the blinker task
Blinker   BuiltIn(LED_BUILTIN,100,50);
Blinker   Blinker1(kOutPinSlowBlink,500);
Blinker   Blinker2(kOutPinFastBlink,1500,500);

// 2 Instances of the PushBut task
PushBut   PushA(kInPinA,&GPushA);
PushBut   PushB(kInPinB,&GPushB);

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
