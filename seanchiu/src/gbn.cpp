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
// Shared Variable
int WINDOW_SIZE;
// A variables
float TIMEOUT = 20.0;
int send_base;
int next_seq_num;
struct pkt** in_transit;
std::queue<msg> buffer;
// B variables
int rcv_base;

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
  if(in_transit[next_seq_num] != NULL) {
    // Window is full
    buffer.push(message);
  } else {
    // Space to send in window
    int payload_checksum = 0;
    struct pkt* to_send = (struct pkt*)malloc(sizeof(struct pkt));
    to_send->seqnum = next_seq_num;
    to_send->acknum = 0;
    for(int i = 0; i < 20; i++) {
      to_send->payload[i] = message.data[i];
      payload_checksum += (int)message.data[i];
    }
    // calculate checksum for pkt
    to_send->checksum = to_send->seqnum + to_send->acknum + payload_checksum;
    // store packet in case need to resent, andthen send packet
    in_transit[next_seq_num] = to_send;
    tolayer3(0, *to_send);
    // start timer for new packet IF it is the oldest packet or base backet
    if(send_base == next_seq_num) {
      starttimer(0, TIMEOUT);
    }
    next_seq_num = (next_seq_num + 1) % WINDOW_SIZE;
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  // Verify Checksum
  int packet_payload_checksum = 0;
  for(int i = 0; i < 20; i++) {
    packet_payload_checksum += (int)packet.payload[i];
  }
  int checksum = packet.seqnum + packet.acknum + packet_payload_checksum;
  if(checksum == packet.checksum) {
    if(in_transit[send_base] != NULL) {
      if(in_transit[send_base]->seqnum == packet.acknum) {
        stoptimer(0);
        free(in_transit[send_base]);
        in_transit[send_base] = NULL;
        send_base = (send_base + 1) % WINDOW_SIZE;
        if(in_transit[send_base] != NULL) {
          //printf("Starting timer heere!\n");
          starttimer(0, TIMEOUT);
        }
        if(buffer.size() > 0) {
          // Still messages in buffer that needs to be sent
          struct msg next_msg = buffer.front();
          struct pkt* next_packet = (struct pkt*) malloc(sizeof(struct pkt));
          next_packet->seqnum = next_seq_num;
          next_packet->acknum = 0;
          int payload_checksum = 0;
          for(int i = 0; i < 20; i++) {
            next_packet->payload[i] = next_msg.data[i];
            payload_checksum += next_msg.data[i];
          }
          next_packet->checksum = next_packet->seqnum + next_packet->acknum + payload_checksum;
          tolayer3(0, *next_packet);
          in_transit[next_seq_num] = next_packet;
          if(send_base == next_seq_num) {
            //printf("Attempting to start timer after!\n");
            starttimer(0, TIMEOUT);
          }
          next_seq_num = (next_seq_num + 1) % WINDOW_SIZE;
          buffer.pop();
        }
      } else {
        stoptimer(0);
        while(in_transit[send_base]->seqnum != packet.acknum) {
          free(in_transit[send_base]);
          in_transit[send_base] = NULL;
          send_base = (send_base + 1) % WINDOW_SIZE;
        }
        if(in_transit[send_base] != NULL) {
          //printf("Starting timer heere!\n");
          starttimer(0, TIMEOUT);
        }
        if(buffer.size() > 0) {
          // Still messages in buffer that needs to be sent
          struct msg next_msg = buffer.front();
          struct pkt* next_packet = (struct pkt*) malloc(sizeof(struct pkt));
          next_packet->seqnum = next_seq_num;
          next_packet->acknum = 0;
          int payload_checksum = 0;
          for(int i = 0; i < 20; i++) {
            next_packet->payload[i] = next_msg.data[i];
            payload_checksum += next_msg.data[i];
          }
          next_packet->checksum = next_packet->seqnum + next_packet->acknum + payload_checksum;
          tolayer3(0, *next_packet);
          in_transit[next_seq_num] = next_packet;
          if(send_base == next_seq_num) {
            //printf("Attempting to start timer after!\n");
            starttimer(0, TIMEOUT);
          }
          next_seq_num = (next_seq_num + 1) % WINDOW_SIZE;
          buffer.pop();
        }
      }
    }
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  // Packet at send_base timed out, need to resend send_base and every packet after that
  tolayer3(0, *in_transit[send_base]);
  int curr_idx = (send_base + 1) % WINDOW_SIZE;
  while(curr_idx != next_seq_num - 1) {
    tolayer3(0, *in_transit[send_base]);
    curr_idx = (curr_idx + 1) % WINDOW_SIZE;
  }
  starttimer(0, TIMEOUT);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  WINDOW_SIZE = getwinsize();
  next_seq_num = 0;
  send_base = 0;
  in_transit = new struct pkt*[WINDOW_SIZE];
  for(int i = 0; i < WINDOW_SIZE; i++) {
    in_transit[i] = NULL;
  }
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
  /// Verify checksum
  int packet_payload_checksum = 0;
  for(int i = 0; i < 20; i++) {
    packet_payload_checksum += (int)packet.payload[i];
  }
  int checksum = packet.seqnum + packet.acknum + packet_payload_checksum;
  // Checksum OK proceed
  if(checksum == packet.checksum) {
    printf("Packet Verified! \n");
    printf("Packet Seq Num : %d\n ", packet.seqnum);
    printf("Expected Seq Num : %d\n ", rcv_base);
    if(packet.seqnum == rcv_base) {
      tolayer5(1, packet.payload);
      struct pkt ack;
      ack.seqnum = packet.seqnum;
      ack.acknum = rcv_base;
      int payload_checksum = 0;
      for(int i = 0; i < 20; i++) {
        ack.payload[i] = packet.payload[i];
        payload_checksum += packet.payload[i];
      }
      ack.checksum = ack.seqnum + ack.acknum + payload_checksum;
      tolayer3(1, ack);
      rcv_base = (rcv_base + 1) % WINDOW_SIZE;
    } else {
      struct pkt ack;
      ack.seqnum = packet.seqnum;
      ack.acknum = (rcv_base - 1) % WINDOW_SIZE;
      int payload_checksum = 0;
      for(int i = 0; i < 20; i++) {
        ack.payload[i] = packet.payload[i];
        payload_checksum += packet.payload[i];
      }
      ack.checksum = ack.seqnum + ack.acknum + payload_checksum;
      tolayer3(1, ack);
    }
  }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  rcv_base = 0;
}
