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
// Shared variables
int WINDOW_SIZE;
// A variables
float TIMEOUT = 30.0;
int send_base;
int next_seq_num;
struct pkt** in_transit;
float* send_time;
std::queue<msg> buffer;
std::queue<int> timer_order;
struct pkt** ack_buffer;
// B variables
int rcv_base;
int* duplicates;
struct pkt** received_buffer;
/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
  //printf("Received Message %s\n", message.data);
  if(in_transit[next_seq_num] != NULL) {
    // No space in window
    buffer.push(message);
  } else {
    // Space to send packet
    int payload_checksum = 0;
    struct pkt* to_send = (struct pkt*)malloc(sizeof(struct pkt));
    to_send->seqnum = next_seq_num;
    to_send->acknum = 0;
    for(int i = 0; i < 20; i++) {
      to_send->payload[i] = message.data[i];
      payload_checksum += (int)message.data[i];
    }
    // Calculate checksum for pkt
    to_send->checksum = to_send->seqnum + to_send->acknum + payload_checksum;
    // Store packet in case need to resend, then send packet to B
    in_transit[next_seq_num] = to_send;
    tolayer3(0, *to_send);
    // Start timer for new packet IF it is the oldest packet or base packet
    if(send_base == next_seq_num) {
      starttimer(0, TIMEOUT);
    }
    // Need to keep track of time for when we have to interrupt next
    send_time[next_seq_num] = get_sim_time();
    timer_order.push(next_seq_num);
    // Increment next_seq_num
    next_seq_num = (next_seq_num + 1) % WINDOW_SIZE;
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  for(int i = 0; i < timer_order.size(); i++) {
    // printf("Timer order at position %d is : %d\n", i, timer_order.front());
    timer_order.push(timer_order.front());
    timer_order.pop();
  }
  // Calculate Checksum
  int packet_payload_checksum = 0;
  for(int i = 0; i < 20; i++) {
    packet_payload_checksum += (int)packet.payload[i];
  }
  int checksum = packet.seqnum + packet.acknum + packet_payload_checksum;
  // Make sure checksum is correct
  if(checksum == packet.checksum) {
    // Checksum is OK continue
    // If there is a packet we are waiting for
    if(in_transit[packet.acknum] != NULL) {
      if(packet.acknum == in_transit[send_base]->seqnum) {
        bool correct_payload = true;
        for(int i = 0; i < 20; i++) {
          if(in_transit[packet.acknum]->payload[i] != packet.payload[i]) {
            correct_payload = false;
            break;
          }
        }
        if(!correct_payload) {
          return;
        }
        // Next ack received, move window and keep on doing for in-order packets
        struct pkt curr_pkt = packet;
        while(in_transit[send_base] != NULL && curr_pkt.acknum == in_transit[send_base]->seqnum) {
          //printf("Received Ack : %d\n", curr_pkt.acknum);
          //printf("Expected Ack : %d\n", in_transit[send_base]->seqnum);
          //printf("Timer at top : %d\n", timer_order.front());
          stoptimer(0);
          free(in_transit[send_base]);
          in_transit[send_base] = NULL;
          bool acked_before = false;
          if(ack_buffer[send_base] != NULL) {
            free(ack_buffer[send_base]);
            ack_buffer[send_base] = NULL;
            acked_before = true;
          }
          send_base = (send_base + 1) % WINDOW_SIZE;
          if(in_transit[send_base] != NULL) {
            starttimer(0, (send_time[send_base] + TIMEOUT) - get_sim_time());
          }
          //printf("Next awaiting seq num : %d\n", send_base);
          if(in_transit[send_base] == NULL) {
            //printf("Nothing is waiting!\n");
          } else {
            //printf("Packet is waiting with seq num : %d\n", in_transit[send_base]->seqnum);
          }
          if(ack_buffer[send_base] == NULL) {
            //printf("Nothing here!\n");
          } else {
            //printf("Next packet that is bufferd with seq num : %d\n", ack_buffer[send_base]->seqnum);
          }
          if(!acked_before) {
            if(timer_order.front() != curr_pkt.acknum) {
              int timer_order_front = timer_order.front();
              timer_order.push(timer_order_front);
              timer_order.pop();
              while(timer_order.front() != timer_order_front) {
                if(timer_order.front() == curr_pkt.acknum) {
                  //printf("Removed %d from top of timer order\n", timer_order.front());
                  timer_order.pop();
                } else {
                  timer_order.push(timer_order.front());
                  timer_order.pop();
                }
              }
            } else {
              timer_order.pop();
              if(timer_order.size() > 0) {
                //printf("Next time out packet is : %d\n", timer_order.front());
              }
            }
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
            in_transit[next_seq_num] = next_packet;
            tolayer3(0, *next_packet);
            // Start timer for new packet IF it is the oldest packet or base packet
            if(send_base == next_seq_num) {
              starttimer(0, TIMEOUT);
            }
            // Need to keep track of time for when we have to interrupt next
            send_time[next_seq_num] = get_sim_time();
            timer_order.push(next_seq_num);
            // Increment next_seq_num
            buffer.pop();
            next_seq_num = (next_seq_num + 1) % WINDOW_SIZE;
          }
          if(ack_buffer[send_base] != NULL) {
            curr_pkt = *ack_buffer[send_base];
          }
        }
      } else {
        // Ack not in-order need to buffer, need to make sure that we are not accepting duplicate acks
        if(ack_buffer[packet.acknum] == NULL) {
          if(in_transit[packet.acknum] != NULL) {
            bool correct_payload = true;
            for(int i = 0; i < 20; i++) {
              if(in_transit[packet.acknum]->payload[i] != packet.payload[i]) {
                correct_payload = false;
                break;
              }
            }
            if(correct_payload) {
              //printf("Buffering packet with seq num : %d\n", packet.acknum);
              //printf("Buffering packet with payload : %s\n", packet.payload);
              struct pkt* packet_to_buffer = (struct pkt*) malloc(sizeof(struct pkt));
              packet_to_buffer->seqnum = packet.seqnum;
              packet_to_buffer->acknum = packet.acknum;
              int payload_checksum = 0;
              for(int i = 0; i < 20; i++) {
                packet_to_buffer->payload[i] = packet.payload[i];
              }
              packet_to_buffer->checksum = packet.checksum;
              ack_buffer[packet.acknum] = packet_to_buffer;
              if(timer_order.front() != packet.acknum) {
                int timer_order_front = timer_order.front();
                timer_order.push(timer_order_front);
                timer_order.pop();
                while(timer_order.front() != timer_order_front) {
                  if(timer_order.front() == packet.acknum) {
                    timer_order.pop();
                  } else {
                    timer_order.push(timer_order.front());
                    timer_order.pop();
                  }
                }
              } else {
                timer_order.pop();
              }
            }
          }
        }
      }
    }
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  // When timer goes off, resend packet then add to back of queue and move onto next timer
  int timed_out_pkt = timer_order.front();
  if(ack_buffer[timed_out_pkt] == NULL) {
    timer_order.pop();
    timer_order.push(timed_out_pkt);
    //printf("here1\n");
    send_time[timed_out_pkt] = get_sim_time();
    //printf("Send Base : %d\n", send_base);
    //printf("Packet that timed out : %d\n", timed_out_pkt);
    tolayer3(0, *in_transit[timed_out_pkt]);
    //printf("here3\n");
    starttimer(0, (send_time[timer_order.front()] + TIMEOUT) - get_sim_time());
  } else {
    timer_order.pop();
    starttimer(0, (send_time[timer_order.front()] + TIMEOUT) - get_sim_time());
  }
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
  ack_buffer = new struct pkt*[WINDOW_SIZE];
  for(int i = 0; i < WINDOW_SIZE; i++) {
    ack_buffer[i] = NULL;
  }
  send_time = new float[WINDOW_SIZE];
  for(int i = 0; i < WINDOW_SIZE; i++) {
    send_time[i] = 0.0;
  }
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
  // Calculate checksum
  int packet_payload_checksum = 0;
  for(int i = 0; i < 20; i++) {
    packet_payload_checksum += (int)packet.payload[i];
  }
  int checksum = packet.seqnum + packet.acknum + packet_payload_checksum;
  // Make sure checksum is correct
  if(checksum == packet.checksum) {
    // Checksum OK proceed
    // Check to make sure we are not receiving duplicates
    if(duplicates[packet.seqnum] != packet.checksum && received_buffer[packet.seqnum] == NULL) {
      // Check to make sure that seqnum is in-order
      if(packet.seqnum == rcv_base) {
        // In-order so deliver and deliver next in-order packets
        // Deliver just received packet
        tolayer5(1, packet.payload);
        duplicates[packet.seqnum] = packet.checksum;
        rcv_base = (rcv_base + 1) % WINDOW_SIZE;
        // Deliver the buffered packets
        while(received_buffer[rcv_base] != NULL) {
          // Next packet is already buffered, deliver
          struct pkt next = *received_buffer[rcv_base];
          tolayer5(1, next.payload);
          duplicates[next.seqnum] = next.checksum;
          free(received_buffer[rcv_base]);
          received_buffer[rcv_base] = NULL;
          rcv_base = (rcv_base + 1) % WINDOW_SIZE;
        }
      } else {
        // Not in order, buffer for later
        struct pkt* buffer_for_later = (struct pkt*)malloc(sizeof(struct pkt));
        buffer_for_later->seqnum = packet.seqnum;
        buffer_for_later->acknum = packet.acknum;
        for(int i = 0; i < 20; i++) {
            buffer_for_later->payload[i] = packet.payload[i];
        }
        buffer_for_later->checksum = packet.checksum;
        received_buffer[buffer_for_later->seqnum] = buffer_for_later;
      }
      // We always ack with packet seq num when received valid
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
      // Maybe sending too many acks? We only want to send ack if it is in the window N-1 to N + 1
      if(duplicates[packet.seqnum] == packet.checksum) {
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
      }
      if(received_buffer[packet.seqnum] != NULL) {
        bool same_packet = true;
        for(int i = 0; i < 20; i++) {
          if(packet.payload[i] != received_buffer[packet.seqnum]->payload[i]) {
            same_packet = false;
            break;
          }
        }
        if(same_packet) {
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
        }
      }
    }
  }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  rcv_base = 0;
  duplicates = new int[WINDOW_SIZE];
  for(int i = 0; i < WINDOW_SIZE; i++) {
    duplicates[i] = 0;
  }
  received_buffer = new struct pkt*[WINDOW_SIZE];
  for(int i = 0; i < WINDOW_SIZE; i++) {
    received_buffer[i] = NULL;
  }
}
