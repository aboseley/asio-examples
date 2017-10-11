#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

boost::asio::io_service ios;
boost::posix_time::seconds interval(1);
boost::asio::deadline_timer timer(ios, interval);

void data_send(void)
{
    std::cout << "omg sent" << std::endl;
}

void data_rec(struct can_frame& rec_frame,boost::asio::posix::basic_stream_descriptor<>& stream)
{
     std::cout << std::hex << rec_frame.can_id << "  ";
     for(int i=0;i<rec_frame.can_dlc;i++)
     {
         std::cout << std::hex << int(rec_frame.data[i]) << " ";
     }
     std::cout << std::dec << std::endl;
     stream.async_read_some(boost::asio::buffer(&rec_frame, sizeof(rec_frame)),boost::bind(data_rec,boost::ref(rec_frame),boost::ref(stream)));
}

void timeout(const boost::system::error_code& error)
{
  if (!error)
  {
     std::cout << "timeout :" << timer.expires_at() << std::endl;

     timer.expires_at(timer.expires_at() + interval);
     timer.async_wait(timeout);
  }
}

int main(void)
{
    struct sockaddr_can addr;
    struct can_frame frame;
    struct can_frame rec_frame;
    struct ifreq ifr;

    int natsock = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    strcpy(ifr.ifr_name, "can0");
    ioctl(natsock, SIOCGIFINDEX, &ifr);

    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex; 
    if(bind(natsock,(struct sockaddr *)&addr,sizeof(addr))<0)
    {
        perror("Error in socket bind");
        return -2;
    }

    frame.can_id  = 0x123;
    frame.can_dlc = 2;
    frame.data[0] = 0x11;
    frame.data[1] = 0x23;

    boost::asio::posix::basic_stream_descriptor<> stream(ios);
    stream.assign(natsock);

    stream.async_read_some(boost::asio::buffer(&rec_frame, sizeof(rec_frame)),boost::bind(data_rec,boost::ref(rec_frame),boost::ref(stream)));
    stream.async_write_some(boost::asio::buffer(&frame, sizeof(frame)),boost::bind(data_send));
    timer.async_wait(timeout);

    ios.run();
}
