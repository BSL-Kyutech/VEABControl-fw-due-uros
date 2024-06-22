//===============================================
// Flags that need to be set

// Leave the one you want and comment th other out
#define VEAB
//#define POT

// Set topic names to avoid duplication
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

//#include <std_msgs/msg/int32.h>
//#include <std_msgs/msg/u_int16.h>
#include <std_msgs/msg/u_int16_multi_array.h>

std_msgs__msg__UInt16MultiArray msg;
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
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}

unsigned int desired[CH_NUM];
unsigned int realized[CH_NUM];


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
    for (int i = 0; i < CH_NUM; i++) {

      msg.data.data[i] = realized[i];
#ifdef VEAB
      analogWrite(i+PWM_PIN_LOWEST, (int)desired[i]);
#endif
    }
    RCSOFTCHECK(rcl_publish(&publisher, &msg, NULL));
  }
}

#ifdef VEAB
void subscription_callback(const void * msgin)
{  
  const std_msgs__msg__UInt16MultiArray * msg = (const std_msgs__msg__UInt16MultiArray *)msgin;
  for (int i = 0; i < CH_NUM; i++) {
    desired[i] = msg->data.data[i];
  }
  //msg->data[]
  //digitalWrite(LED_PIN, (msg->data == 0) ? LOW : HIGH);  
}
#endif

void setup() {
  //Serial.begin(115200);
  //set_microros_serial_transports(Serial);
  set_microros_transports();
  
  // configure LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // configure PWM pins
  for (int pin = PWM_PIN_LOWEST; pin < PWM_PIN_LOWEST+12; pin++) {
    pinMode(pin, OUTPUT);
  }

  // configure Analog pins
  for (int pin = ANALOG_PIN_LOWEST; pin < ANALOG_PIN_LOWEST+12; pin++) {
    pinMode(pin, INPUT);
  }

  // initialize variables
  msg.data.capacity = CH_NUM;
  msg.data.size = CH_NUM;
  msg.data.data = (uint16_t*)malloc(msg.data.capacity * sizeof(uint16_t));

  for (int i = 0; i < CH_NUM; i++) {
    desired[i] = 0;
    realized[i] = 0;
    msg.data.data[i] = 0;
  }

  delay(2000);

  allocator = rcl_get_default_allocator();

  // create init_options
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  // create node
  RCCHECK(rclc_node_init_default(&node, "micro_ros_arduino_node", "", &support));

#ifdef VEAB
  // create subscriber
  RCCHECK(rclc_subscription_init_default(
  //RCCHECK(rclc_subscription_init_best_effort(
    &subscriber,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, UInt16MultiArray),
    SUB_TOPICNAME));
#endif

  // create publisher
  RCCHECK(rclc_publisher_init_default(
  //RCCHECK(rclc_publisher_init_best_effort(
    &publisher,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, UInt16MultiArray),
    PUB_TOPICNAME));

  // create timer,
  const unsigned int timer_timeout = 8;
  RCCHECK(rclc_timer_init_default(
    &timer,
    &support,
    RCL_MS_TO_NS(timer_timeout),
    timer_callback));

  // create executor
  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
  RCCHECK(rclc_executor_add_timer(&executor, &timer));

  //msg.data = 0;
}

void loop() {

  // ADC
  for (int i = 0; i < CH_NUM; i++) {
    realized[i] = analogRead(i+ANALOG_PIN_LOWEST);
  }
  RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(1)));

}
