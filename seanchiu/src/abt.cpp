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
float TIMEOUT = 20.0;
int next_seq;
int expected_seq_num;
std::queue<msg> buffer;
struct pkt* in_transit = NULL;
/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
  if(in_transit != NULL) {
    // If already waiting for message, add to buffer
    buffer.push(message);
  } else {
    // If not waiting for message, we can just send it
    int payload_checksum = 0;
    struct pkt* to_send = (struct pkt*)malloc(sizeof(struct pkt));
    to_send->seqnum = next_seq;
    to_send->acknum = 0;
    for(int i = 0; i < 20; i++) {
      to_send->payload[i] = message.data[i];
      payload_checksum += (int)message.data[i];
    }
    // calculate checksum for pkt
    to_send->checksum = to_send->seqnum + to_send->acknum + payload_checksum;
    // store packet in case need to resend, and then send packet.
    in_transit = to_send;
    // printf("%d\n", in_transit->seqnum);
    tolayer3(0, *to_send);
    // start timer for packet
    starttimer(0, TIMEOUT);
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  // Verify checksum
  // printf("Received Ack\n");
  int packet_payload_checksum = 0;
  for(int i = 0; i < 20; i++) {
    packet_payload_checksum += (int)packet.payload[i];
  }
  int checksum = packet.seqnum + packet.acknum + packet_payload_checksum;
  if(checksum == packet.checksum) {
    // printf("Checksum OK\n");
    // Checksum OK proceed
    // printf("Expected ack num : %d, Actual ack num : %d\n", in_transit->seqnum, packet.acknum);
     if(in_transit->seqnum == packet.acknum) {
       // printf("Correct ack\n");
       stoptimer(0);
       // This is the ack we were waiting for
       printf("Queue size : %d\n", buffer.size());
       if(buffer.size() > 0) {
         // printf("Sending next message\n");
         // Still messages in buffer that needs to be sent
         struct msg next_msg = buffer.front();
         struct pkt* next_packet = (struct pkt*) malloc(sizeof(struct pkt));
         next_packet->seqnum = next_seq;
         next_packet->acknum = 0;
         int payload_checksum = 0;
         for(int i = 0; i < 20; i++) {
           next_packet->payload[i] = next_msg.data[i];
           payload_checksum += next_msg.data[i];
         }
         next_packet->checksum = next_packet->seqnum + next_packet->acknum + payload_checksum;
         tolayer3(0, *next_packet);
         free(in_transit);
         in_transit = next_packet;
         starttimer(0, TIMEOUT);
         if(next_seq == 0) {
           next_seq = 1;
         } else {
           next_seq = 0;
         }
         buffer.pop();
       } else {
         // No more messages in buffer
         // printf("Nothing in buffer! \n");
         in_transit = NULL;
       }
     }
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  tolayer3(0, *in_transit);
  starttimer(0, TIMEOUT);
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
  int packet_payload_checksum = 0;
  for(int i = 0; i < 20; i++) {
    packet_payload_checksum += (int)packet.payload[i];
  }
  int checksum = packet.seqnum + packet.acknum + packet_payload_checksum;
  if(checksum == packet.checksum) {
    printf("Expected Seq Num : %d, Packed Seq Num : %d", expected_seq_num, packet.seqnum);
    if(packet.seqnum != expected_seq_num) {
      struct pkt ack;
      ack.seqnum = packet.seqnum;
      ack.acknum = packet.seqnum;
      int payload_checksum = 0;
      for(int i = 0; i < 20; i++) {
        ack.payload[i] = packet.payload[i];
        payload_checksum += packet.payload[i];
      }
      ack.checksum = ack.seqnum + ack.acknum + payload_checksum;
      tolayer3(1, ack);
    } else {
      // Checksum OK proceed
      tolayer5(1, packet.payload);
      struct pkt ack;
      ack.seqnum = packet.seqnum;
      ack.acknum = packet.seqnum;
      int payload_checksum = 0;
      for(int i = 0; i < 20; i++) {
        ack.payload[i] = packet.payload[i];
        payload_checksum += packet.payload[i];
      }
      ack.checksum = ack.seqnum + ack.acknum + payload_checksum;
      tolayer3(1, ack);
      if(expected_seq_num == 0) {
        expected_seq_num = 1;
      } else {
        expected_seq_num = 0;
      }
    }
  }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  expected_seq_num = 0;
}
