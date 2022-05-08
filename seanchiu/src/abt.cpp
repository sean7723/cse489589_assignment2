#include "../include/simulator.h"
#include <queue>
#include <string.h>
#include <stdio.h>
#include <iostream>
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
int next_seq;
std::queue<msg> buffer;
struct pkt* in_transit;
/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
  if(in_transit != NULL) {
    // If already waiting for message, add to buffer
    buffer.push(message);
  } else {
    // If not waiting for message, we can just send it
    struct pkt to_send;
    to_send.seqnum = next_seq;
    to_send.acknum = 0;
    strncpy(message.data, to_send.payload, 20);
    // calculate checksum for pkt
    int payload_converted = atoi(to_send.payload);
    to_send.checksum = to_send.seqnum + to_send.acknum + payload_converted;
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{

}

/* called when A's timer goes off */
void A_timerinterrupt()
{

}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  next_seq = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{

}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{

}
