#include "contiki.h"
#include "net/rime.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "node-id.h"
#include <stdio.h>

#define MAX_RETRANSMISSIONS 4

/*---------------------------------------------------------------------------*/
PROCESS(test_runicast_process, "runicast test");
AUTOSTART_PROCESSES(&test_runicast_process);
/*---------------------------------------------------------------------------*/
static void recv_runicast(struct runicast_conn *c, rimeaddr_t *from, uint8_t seqno);

#define RUNICAST_CHANNEL  120


static void sent_runicast(struct runicast_conn *c, rimeaddr_t *to, uint8_t retransmissions);
static void timedout_runicast(struct runicast_conn *c, rimeaddr_t *to, uint8_t retransmissions);
static const struct runicast_callbacks runicast_callbacks = {recv_runicast, sent_runicast, timedout_runicast};
static struct runicast_conn runicast;

static struct ctimer ledTimer;

static struct timeMessage tmReceived;
static struct timeMessage tmSent;

static clock_time_t rtt;

struct timeMessage {
	clock_time_t time;
	unsigned short originator;
	int isAnswer;
};

static void timerCallback_turnOffLeds()
{
  leds_off(LEDS_BLUE);
}

/* this function has been defined to be called when a unicast is being received */
static void recv_runicast(struct runicast_conn *c, rimeaddr_t *from, uint8_t seqno)
{
  printf("runicast message received from %d.%d, seqno %d\n", from->u8[0], from->u8[1], seqno);

  /* from the packet we have just received, read the data and write it into the
   * struct tmReceived we have declared and instantiated above (line 26)
   */
  packetbuf_copyto(&tmReceived);
    printf("time received = %d clock ticks", (uint16_t)tmReceived.time);
    printf(" = %d secs ", (uint16_t)tmReceived.time / CLOCK_SECOND);
    printf("%d millis ", (1000L * ((uint16_t)tmReceived.time  % CLOCK_SECOND)) / CLOCK_SECOND);
    printf("originator = %d\n", tmReceived.originator);
    leds_on(LEDS_BLUE);
    ctimer_set(&ledTimer, CLOCK_SECOND / 8, timerCallback_turnOffLeds, NULL);
  if(!tmReceived.isAnswer == 1){
     /* prepare the unicast packet to be sent. Write the contents of the struct, where we
     * have just written the time and the id into, to the packet we intend to send
     */
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
    /* when calling runicast_send, we have to specify the address as the second argument (a pointer to the defined rimeaddr_t struct)
     * and then also the number of maximum transmissions */
    runicast_send(&runicast, &addr, MAX_RETRANSMISSIONS);
  }else{
    int clockDifference =  clock_time() - (uint16_t)tmReceived.time;
    int secondDifference = clockDifference / CLOCK_SECOND;
    int msDifference = (1000 * ((uint16_t)clockDifference  % CLOCK_SECOND)) / CLOCK_SECOND;
    printf("Timestamp Difference is %d clock cylces which relates to %d s %d ms\n",clockDifference, secondDifference, msDifference);
  }
}

static void sent_runicast(struct runicast_conn *c, rimeaddr_t *to, uint8_t retransmissions)
{
  printf("runicast message sent to %d.%d, retransmissions %d\n", to->u8[0], to->u8[1], retransmissions);
}
static void timedout_runicast(struct runicast_conn *c, rimeaddr_t *to, uint8_t retransmissions)
{
  printf("runicast message timed out when sending to %d.%d, retransmissions %d\n", to->u8[0], to->u8[1], retransmissions);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_runicast_process, ev, data)
{
  PROCESS_EXITHANDLER(runicast_close(&runicast);)

  PROCESS_BEGIN();

  runicast_open(&runicast, RUNICAST_CHANNEL, &runicast_callbacks);

  SENSORS_ACTIVATE(button_sensor);
  while(1) {
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
  	    /* when calling runicast_send, we have to specify the address as the second argument (a pointer to the defined rimeaddr_t struct)
  	     * and then also the number of maximum transmissions */
  	    runicast_send(&runicast, &addr, MAX_RETRANSMISSIONS);
  }

  PROCESS_END();
}


/* (to be submitted with ILIAS)
 *
 * Compile and flash the above code to your nodes. Read and understand the code. Check
 * the contiki documentation for the runicast primitive and understand how the mechanism
 * with the callbacks works.
 *
 * Runicast = reliable unicast. That means the node sending a frame waits for an acknowledgement.
 * In case the acknowledgement does not arrive after the specified time, it sends the frame again
 * In total it sends it MAX_RETRANSMISSIONS times. You can alter MAX_RETRANSMISSIONS
 *
 * Press the button and observe what happens. Have the receiver plugged in and see
 * what is printed out. The node where the USER button is pressed sends a packet containing
 * its current timestamp (in clock ticks, where 128 ticks = 1s) to the other.
 *
 * Your tasks:
 *
 * a) alter the program above such that the node where the USER button is pressed sends a
 * packet with its timestamp (is already done above) and THEN gets back a runicast packet with the
 * timestamp it has initially written into the first packet. Based on this packet, compute the
 * Route-trip time at the node initiating the packet exchange and print it to the serial interface.
 *make
 * b) compare the latency with that from unicast exercise. Explain the differences (add your answer as comments or separate file).
 */

/*
* ANSWER: The round-trip time is usually 5 to 6 clock cycles wich relates to about 40 ms
*/