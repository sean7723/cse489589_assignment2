#include "../include/simulator.h"
#include <queue>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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
float TIMEOUT = 10.0;
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
    memcpy(message.data, to_send.payload, sizeof(message.data));
    // calculate checksum for pkt
    to_send.checksum = to_send.seqnum + to_send.acknum + atoi(to_send.payload);
    // store packet in case need to resend, and then send packet.
    in_transit = &to_send;
    tolayer3(0, to_send);
    // start timer for packet
    starttimer(0, TIMEOUT);
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  // Verify checksum
  int checksum = packet.seqnum + packet.acknum + atoi(packet.payload);
  if(checksum == packet.checksum) {
    // Checksum OK proceed
     if(in_transit->seqnum == packet.acknum) {
       stoptimer(0);
       // This is the ack we were waiting for
       if(buffer.size() > 0) {
         // Still messages in buffer that needs to be sent
         struct msg next_msg = buffer.front();
         struct pkt next_packet;
         next_packet.seqnum = next_seq;
         next_packet.acknum = 0;
         memcpy(next_msg.data, next_packet.payload, sizeof(next_msg.data));
         buffer.pop();
         next_packet.checksum = next_packet.seqnum + next_packet.acknum + atoi(next_packet.payload);
         tolayer3(0, next_packet);
         in_transit = &next_packet;
         starttimer(0, TIMEOUT);
         if(next_seq == 0) {
           next_seq = 1;
         } else {
           next_seq = 0;
         }
       } else {
         // No more messages in buffer
         in_transit = NULL;
       }
     }
  }
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
  // Verify checksum
  int checksum = packet.seqnum + packet.acknum + atoi(packet.payload);
  if(checksum == packet.checksum) {
    // Checksum OK proceed
    tolayer5(1, packet.payload);
    struct pkt ack;
    ack.seqnum = packet.seqnum;
    ack.acknum = packet.seqnum;
    memcpy(packet.payload, ack.payload, 20);
    ack.checksum = ack.seqnum + ack.acknum + atoi(ack.payload);
    tolayer3(1, ack);
  }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{

}
