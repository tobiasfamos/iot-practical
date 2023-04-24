
#include "contiki.h"
#include "net/rime.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "node-id.h"
#include "sys/rtimer.h"
#include <stdio.h>


struct timeMessage {
	clock_time_t time;
	unsigned short originator;
	int isAnswer;
};

/* two timeMessage struct declaration/instantiations */
static struct timeMessage tmReceived;
static struct timeMessage tmSent;

void print_temperature_binary_to_float(uint16_t temp) {
	printf("%d.%d", (temp / 10 - 396) / 10, (temp / 10 - 396) % 10);
}


static struct ctimer leds_off_timer_send;

/* Timer callback turns off the blue led */
static void timerCallback_turnOffLeds()
{
  leds_off(LEDS_BLUE);
}

/*---------------------------------------------------------------------------*/
PROCESS(example_unicast_process, "RTT using Rime Unicast Primitive");
AUTOSTART_PROCESSES(&example_unicast_process);
/*---------------------------------------------------------------------------*/

static void recv_uc(struct unicast_conn *c, const rimeaddr_t *from);
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc;

/* two clock_time_t declaration/instantiations */
static clock_time_t rtt;

/* this function has been defined to be called when a unicast is being received */
static void recv_uc(struct unicast_conn *c, const rimeaddr_t *from)
{

  printf("unicast message received from %d.%d\n", from->u8[0], from->u8[1]);
  /* turn on blue led */
  leds_on(LEDS_BLUE);
  /* set the timer "leds_off_timer" to 1/8 second */
  ctimer_set(&leds_off_timer_send, CLOCK_SECOND / 8, timerCallback_turnOffLeds, NULL);

  /* from the packet we have just received, read the data and write it into the
   * struct tmReceived we have declared and instantiated above (line 16)
   */
  packetbuf_copyto(&tmReceived);

  /* print the contents of the received packet */
  printf("time received = %d clock ticks", (uint16_t)tmReceived.time);
  printf(" = %d secs ", (uint16_t)tmReceived.time / CLOCK_SECOND);
  printf("%d millis ", (1000L * ((uint16_t)tmReceived.time  % CLOCK_SECOND)) / CLOCK_SECOND);
  printf("originator = %d\n", tmReceived.originator);

    if(!tmReceived.isAnswer == 1){
        tmSent.time = tmReceived.time;
        tmSent.originator = node_id;
        tmSent.isAnswer = 1;
        packetbuf_copyfrom(&tmSent, sizeof(tmSent));

        rimeaddr_t addr;

        if(node_id == 1){
            addr.u8[0] = 33;
        }else{
            addr.u8[0] = 1;
        }
        addr.u8[1] = 0;

         if(!rimeaddr_cmp(&addr, &rimeaddr_node_addr)) {
              /* when calling unicast_send, we have to specify the address as the second argument (a pointer to the defined rimeaddr_t struct) */
              unicast_send(&uc, &addr);
        }
        printf("sending packet to %u\n", addr.u8[0]);
    }
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_unicast_process, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc);)

  PROCESS_BEGIN();

  unicast_open(&uc, 146, &unicast_callbacks);

  SENSORS_ACTIVATE(button_sensor);

  while(1){
	  PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);

	  /* when the button is pressed, read the current time and write it to the
	   * previously declared tmSent struct */
	  tmSent.time = clock_time();
	  /* write the id of then node where the button is pressed into the packet */
	  tmSent.originator = node_id;
        tmSent.isAnswer = 0;


	  /* prepare the unicast packet to be sent. Write the contents of the struct, where we
	   * have just written the time and the id into, to the packet we intend to send
	   */
	  packetbuf_copyfrom(&tmSent, sizeof(tmSent));


	  /* specify the address of the unicast */
	  rimeaddr_t addr;

        if(node_id == 1){
            addr.u8[0] = 33;
        }else {
            addr.u8[0] = 1;
        }
	  	addr.u8[1] = 0;

	  if(!rimeaddr_cmp(&addr, &rimeaddr_node_addr)) {
		  /* when calling unicast_send, we have to specify the address as the second argument (a pointer to the defined rimeaddr_t struct) */
		  unicast_send(&uc, &addr);
	  }
	  printf("sending packet to %u\n", addr.u8[0]);
  }

  SENSORS_DEACTIVATE(button_sensor);


  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/* (to be submitted with ILIAS)
 *
 * Read and understand the above code. Compile and flash the code to your nodes.  Check
 * the contiki documentation for the unicast primitive and understand how the mechanism
 * with the callbacks works.
 *
 * Choose one node as the sender and one as the receiver of the unicast message
 * Press the button on the sender and observe what happens. Have the receiver plugged in and see
 * what is printed out. The node where the USER button is pressed sends a packet containing
 * its current timestamp (in clock ticks, where 128 ticks = 1s) to the other.
 *
 * Your task: alter the program above such that the node where the USER button is pressed sends a
 * packet with its timestamp (is already done above) and THEN gets back a unicast packet with the
 * timestamp it has initially written into the first packet. Based on this packet, compute the
 * Route-trip time at the node initiating the packet exchange and print it to the serial interface.
 * ANSWER: The Received is usually 5 or 6 clock ticks, thus at  about 40 to 40 ms
 */
