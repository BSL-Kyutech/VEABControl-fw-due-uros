//===============================================
// Flags that need to be set

// Leave the one you want and comment th other out
#define VEAB
//#define POT

// Set topic unique names
#define SUB_TOPICNAME "/VEAB1/desired"
#define PUB_TOPICNAME "/VEAB1/realized"
//===============================================

#include <Arduino.h>
#include <micro_ros_arduino.h>

#include <stdio.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <std_msgs/msg/u_int16_multi_array.h>
#include <std_msgs/msg/multi_array_dimension.h>
#include <std_msgs/msg/multi_array_layout.h>

std_msgs__msg__UInt16MultiArray msg_pub;
std_msgs__msg__UInt16MultiArray msg_sub;
rcl_subscription_t subscriber;
rcl_publisher_t publisher;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;
rcl_timer_t timer;

#define LED_PIN 13
#define ANALOG_PIN_LOWEST A0
#define PWM_PIN_LOWEST 2
#define CH_NUM 12

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();}}
#define RCPOLL(fn) { while((fn != RCL_RET_OK)) {} }

#define WDT_KEY (0xA5)

// Since ARM Cortex-M3 does not perform parallel processing, 
// there is no need to specify volatile, but for reusing this code
// and clarifying the meaning, I specify volatile.
volatile uint16_t desired[CH_NUM];
volatile uint16_t realized[CH_NUM];

/*This function is called from init(). If the user does not provide
  this function, then the default action is to disable watchdog.
  This function has to be overriden, otherwise watchdog won't work !! */

void watchdogSetup(void) {
  /*** watchdogDisable (); ***/
}

void error_loop(){
  while(1){
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    delay(100);
  }
}

void timer_callback(rcl_timer_t * timer, int64_t last_call_time)
{
  RCLC_UNUSED(last_call_time);
  if (timer != NULL) {
    // update a message
    for (size_t i = 0; i < CH_NUM; i++) {
      msg_pub.data.data[i] = realized[i];
    }
    // publish
    //RCCHECK(rcl_publish(&publisher, &msg_pub, NULL));
    RCPOLL(rcl_publish(&publisher, &msg_pub, NULL));
  }
}

#ifdef VEAB
void subscription_callback(const void * msgin)
{
  const std_msgs__msg__UInt16MultiArray * msg = (const std_msgs__msg__UInt16MultiArray *)msgin;
  for (size_t i = 0; i < CH_NUM; i++) {
    desired[i] = msg->data.data[i];
  } 
}
#endif


void setup() {

  set_microros_transports();
  
  // configure LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // configure I/O pins
  for (size_t pin = 0; pin < CH_NUM; pin++) {
    pinMode(pin+PWM_PIN_LOWEST, OUTPUT);
    pinMode(pin+ANALOG_PIN_LOWEST, INPUT);
  }

  // create ROS2 message variables
  // NOTE) Due to the minimality of micro-ROS, it easily accesses to wrong address 
  // if a message does not have properly allocated memory volume. The following 
  // initialization is actually crutial. Otherwise, the subscription_callback 
  // smashes the memory.
  msg_pub.data.capacity = CH_NUM;
  msg_pub.data.size = 0;
  msg_pub.data.data = (uint16_t*)malloc(msg_pub.data.capacity * sizeof(uint16_t));
  msg_pub.layout.dim.capacity = 1; // 1-dimentional array: vector
  msg_pub.layout.dim.size = 0;
  msg_pub.layout.dim.data = (std_msgs__msg__MultiArrayDimension*) malloc(msg_pub.layout.dim.capacity * sizeof(std_msgs__msg__MultiArrayDimension));
  for(size_t i = 0; i < msg_pub.layout.dim.capacity; i++){
    msg_pub.layout.dim.data[i].label.capacity = 20;
    msg_pub.layout.dim.data[i].label.size = 0;
    msg_pub.layout.dim.data[i].label.data = (char*) malloc(msg_pub.layout.dim.data[i].label.capacity * sizeof(char));
  }

  msg_sub.data.capacity = CH_NUM;
  msg_sub.data.size = 0;
  msg_sub.data.data = (uint16_t*)malloc(msg_sub.data.capacity * sizeof(uint16_t));
  msg_sub.layout.dim.capacity = 1; // 1-dimentional array: vector
  msg_sub.layout.dim.size = 0;
  msg_sub.layout.dim.data = (std_msgs__msg__MultiArrayDimension*) malloc(msg_sub.layout.dim.capacity * sizeof(std_msgs__msg__MultiArrayDimension));
  for(size_t i = 0; i < msg_sub.layout.dim.capacity; i++){
    msg_sub.layout.dim.data[i].label.capacity = 20;
    msg_sub.layout.dim.data[i].label.size = 0;
    msg_sub.layout.dim.data[i].label.data = (char*) malloc(msg_sub.layout.dim.data[i].label.capacity * sizeof(char));
  }

  // prepare the publish message. initialize values.
  msg_pub.data.size = CH_NUM;
  msg_pub.layout.dim.size = 1;
  msg_pub.layout.dim.data[0].label.data = "channels";
  msg_pub.layout.dim.data[0].label.size = strlen(msg_pub.layout.dim.data[0].label.data);
  msg_pub.layout.dim.data[0].size = CH_NUM;
  msg_pub.layout.dim.data[0].stride = CH_NUM;

  // just give initial values
  for (size_t i = 0; i < CH_NUM; i++) {
    desired[i] = 255; // Using n-ch MOSFET. Set the initial output voltage is 0 [V].
    realized[i] = 0;
    msg_pub.data.data[i] = 0;
    msg_sub.data.data[i] = 0;
  }

  delay(2000);

  //****************************************
  // micro-ROS functions setup
  //****************************************

  // create default allocator
  allocator = rcl_get_default_allocator();

  // create init_options
  RCPOLL(rclc_support_init(&support, 0, NULL, &allocator));

  // create node
  RCPOLL(rclc_node_init_default(&node, "micro_ros_arduino_node", "", &support));

#ifdef VEAB
  // create subscriber
  RCPOLL(rclc_subscription_init_default(
    &subscriber,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, UInt16MultiArray),
    SUB_TOPICNAME));

#endif

  // create publisher
  RCPOLL(rclc_publisher_init_default(
    &publisher,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, UInt16MultiArray),
    PUB_TOPICNAME));

  // create timer,
  const unsigned int timer_timeout = 5;
  RCPOLL(rclc_timer_init_default(
    &timer,
    &support,
    RCL_MS_TO_NS(timer_timeout),
    timer_callback));

  // create executor
#ifdef VEAB
  RCPOLL(rclc_executor_init(&executor, &support.context, 2, &allocator));
  RCPOLL(rclc_executor_add_subscription(&executor, &subscriber, &msg_sub, &subscription_callback, ON_NEW_DATA));
  
#endif
#ifdef POT
  WDT->WDT_CR = WDT_CR_KEY(WDT_KEY) | WDT_CR_WDRSTT; //Restart watchdog
  RCPOLL(rclc_executor_init(&executor, &support.context, 1, &allocator));
#endif
  WDT->WDT_CR = WDT_CR_KEY(WDT_KEY) | WDT_CR_WDRSTT; //Restart watchdog
  RCPOLL(rclc_executor_add_timer(&executor, &timer));
  
  // Enable watchdog.
  WDT->WDT_MR = WDT_MR_WDD(0xFFF) |
                WDT_MR_WDFIEN |  //  Triggers an interrupt or WDT_MR_WDRSTEN to trigger a Reset
                WDT_MR_WDV(256 * 2); // Watchdog triggers a reset or an interrupt after 2 seconds if underflow
  // 2 seconds equal 84000000 * 2 = 168000000 clock cycles
  /* Slow clock is running at 32.768 kHz
    watchdog frequency is therefore 32768 / 128 = 256 Hz
    WDV holds the periode in 256 th of seconds  */
  NVIC_EnableIRQ(WDT_IRQn);
}

void loop() {
  
  WDT->WDT_CR = WDT_CR_KEY(WDT_KEY) | WDT_CR_WDRSTT; //Restart watchdog

  // peform ADC/DAC and update global variables
  for (size_t i = 0; i < CH_NUM; i++) {
    realized[i] = analogRead(i+ANALOG_PIN_LOWEST);
#ifdef VEAB
    analogWrite(i+PWM_PIN_LOWEST, desired[i]);
#endif
  }

  // let executors to run
  //RCCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10)));
  RCPOLL(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10)));

}
