/*!
 * @author
 *  Written by Michael Krzyzaniak at Arizona State 
 *  University's School of Arts, Media + Engineering
 *  and School of Film, Dance and Theatre
 *  in Summer of 2015.
 *
 *  mkrzyzan@asu.edu
 *
 * @unsorted
 */

#include "Network.h"

#include <arpa/inet.h>
#include <unistd.h>    //read, write, close
#include <stdlib.h>    //calloc, free
#include <stdio.h>     //perror
#include <string.h>    //memset

struct opaque_network_struct
{
  int                file_descriptor;
  int                local_file_descriptor;
  struct sockaddr_in remote_address;
  socklen_t          remote_address_length;
  unsigned short     remote_port;
};

/*---------------------------------------------------------------------*/
Network*  net_new               ()
{
  Network* self = calloc(1, sizeof(*self));
  if(self != NULL)
    {
      self->file_descriptor = -1;
      self->local_file_descriptor = -1;
    }
  return self;
}
/*---------------------------------------------------------------------*/
Network*  net_destroy           (Network* self)
{
  if(self != NULL)
    {
      net_disconnect(self);
      free(self);
    }
  return (Network*) NULL;
}

/*---------------------------------------------------------------------*/
void net_init_remote_port(Network* self, uint16_t port)
{
  self->remote_address_length = sizeof(self->remote_address);
  memset(&(self->remote_address), 0, self->remote_address_length);
  self->remote_address.sin_family = AF_INET;
  self->remote_address.sin_port   = htons(port);
}

/*---------------------------------------------------------------------*/
int net_udp_receive(Network* self, void* data, int num_bytes, char returned_addr[16])
{
  struct sockaddr_in remote_address;
  socklen_t          remote_address_length = sizeof(remote_address);
  
  int n = recvfrom(self->file_descriptor, data, num_bytes, 0, 
                 (struct sockaddr*)&remote_address, 
                 &remote_address_length);
                 
  if(returned_addr != NULL)
    {
      char* temp = inet_ntoa(remote_address.sin_addr);
      int len = strlen(temp);
      temp[len] = '\0';
      memcpy(returned_addr, temp, len+1);
    }
  return n;
}

/*-----------------------------------------------------*/
int net_udp_send(Network* self, char* data, int num_bytes, char* ip_address, int port)
{
  net_init_remote_port(self, port);

  if(strcmp(ip_address, "255.255.255.255") == 0)
    self->remote_address.sin_addr.s_addr = htonl(INADDR_BROADCAST);
  else
    self->remote_address.sin_addr.s_addr = inet_addr(ip_address);
  
  int error = sendto(self->file_descriptor, data, num_bytes, 0, 
                    (struct sockaddr *)&(self->remote_address), 
                     self->remote_address_length);
  if(error == -1) perror("unable to send");
  return error;
}

/*---------------------------------------------------------------------*/
int   net_tcp_send (Network* self, void* data, int num_bytes)
{
  return write(self->file_descriptor, data, num_bytes);
}
/*---------------------------------------------------------------------*/
int   net_tcp_receive           (Network* self, void* data     , int num_bytes)
{
  return read(self->file_descriptor, data, num_bytes);
}

/*-----------------------------------------------------*/
int net_bind_port(int fd, char* ip_address, int port)
{
  struct sockaddr_in this_address;
  memset(&this_address, 0, sizeof(this_address));
  this_address.sin_family      = AF_INET;
  this_address.sin_port        = htons(port);
  this_address.sin_addr.s_addr = htonl(inet_addr(ip_address));
  
  return bind(fd, (struct sockaddr*)&this_address, sizeof(this_address));
}

/*---------------------------------------------------------------------*/
BOOL  net_tcp_connect_to_host   (Network* self, char* ip_address, int port)
{
  int error;

  if(self->file_descriptor != -1) net_disconnect(self);

  error = self->local_file_descriptor = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(error == -1)
    {
      perror("Unable to create a TCP socket");
      goto out;
    }

  self->remote_port = port;
  net_init_remote_port(self, port);
  inet_pton(AF_INET, ip_address, &(self->remote_address.sin_addr));
  error = connect(self->local_file_descriptor, (struct sockaddr* )&self->remote_address, 
                  self->remote_address_length);
  if(error == -1)
    {
      perror("Unable to connect to a remote host");
      goto out;
    }
  self->file_descriptor = self->local_file_descriptor;

 out:
  return (error == -1) ? 0 : 1;
}

/*---------------------------------------------------------------------*/
BOOL  net_tcp_receive_connections (Network* self, int port)
{
  int error;
  socklen_t len = sizeof(self->remote_address);

  if(self->local_file_descriptor < 0)// net_disconnect(self);
    {
      error = self->local_file_descriptor = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
      if(error < 0)
        {
          perror("unable to create a TCP socket");
          goto out;
        } 

      self->remote_port = port;
      error = net_bind_port(self->local_file_descriptor, (char*)"0.0.0.0", port);
      if(error != 0) 
        {
          perror("unable to bind the TCP socket");
          net_disconnect(self);
          goto out;
        }

      int max_number_of_clients = 255;
      error = listen(self->local_file_descriptor, max_number_of_clients);
      if(error < 0)
        {
          perror("unable to listen for incoming TCP conections");
          net_disconnect(self);
          goto out;
        }
    }

  if(self->file_descriptor >= 0)
    {
      shutdown(self->file_descriptor, SHUT_RDWR);
      close(self->file_descriptor); 
      self->file_descriptor = -1;
    }

  error = self->file_descriptor = accept(self->local_file_descriptor, (struct sockaddr*)&(self->remote_address), &len);
  if(error < 0)
    perror("unable to recieve a connection from a client");
 
 out:
  return (error < 0) ? 0 : 1;
}

/*---------------------------------------------------------------------*/
BOOL net_udp_connect(Network* self, int receive_port)
{
  int error, b=1;

  if(self->file_descriptor != -1) net_disconnect(self);

  error = self->file_descriptor = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(error == -1)
    {
      perror("unable to create a UDP socket\n"); 
      goto out;
    }

  self->remote_port = receive_port;
  error = net_bind_port(self->file_descriptor, (char*)"0.0.0.0", receive_port);
  if(error == -1)
    { 
      perror("unable to bind UDP socket\n");
      goto out;
    }
    
  error = setsockopt(self->file_descriptor, SOL_SOCKET, SO_BROADCAST, &b, sizeof(b));
    
 out:
  return (error == -1) ? NO : YES;
} 

/*---------------------------------------------------------------------*/
BOOL  net_is_connected           (Network* self)
{
  return (self->file_descriptor != -1);
}

/*---------------------------------------------------------------------*/
void  net_disconnect        (Network* self)
{
  shutdown(self->file_descriptor, SHUT_RDWR);
  close(self->file_descriptor); 
  self->file_descriptor = -1;
  
  shutdown(self->local_file_descriptor, SHUT_RDWR);   
  close(self->local_file_descriptor);
  self->local_file_descriptor = -1;
}
/*---------------------------------------------------------------------*/
void net_get_remote_address(Network* self, char returned_addr[16])
{
  //resides in static memory area
  char* temp = inet_ntoa(self->remote_address.sin_addr);
  int len = strlen(temp);
  temp[len] = '\0';
  memcpy(returned_addr, temp, len+1);
}
/*---------------------------------------------------------------------*/
int   net_get_remote_port   (Network* self)
{
  return self->remote_port;
}


