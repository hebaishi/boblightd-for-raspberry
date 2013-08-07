/*
* boblight
* Copyright (C) Bob 2012
*
* boblight is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* boblight is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "util/log.h"
#include "util/misc.h"
#include "devicespi.h"
#include "util/timeutils.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

CDeviceSPI::CDeviceSPI(CClientsHandler& clients) : m_timer(&m_stop), CDevice(clients)
{
  m_buff = NULL;
  m_buffsize = 0;
  m_fd = -1;
}

void CDeviceSPI::Sync()
{
  if (m_allowsync)
    m_timer.Signal();
}

bool CDeviceSPI::SetupDevice()
{
  m_timer.SetInterval(m_interval);

  m_fd = open(m_output.c_str(), O_RDWR);
  if (m_fd == -1)
  {
    LogError("%s: Unable to open %s: %s", m_name.c_str(), m_output.c_str(), GetErrno().c_str());
    return false;
  }

  int value = SPI_MODE_0;

  if (ioctl(m_fd, SPI_IOC_WR_MODE, &value) == -1)
  {
    LogError("%s: SPI_IOC_WR_MODE: %s", m_name.c_str(), GetErrno().c_str());
    return false;
  }

  if (ioctl(m_fd, SPI_IOC_RD_MODE, &value) == -1)
  {
    LogError("%s: SPI_IOC_RD_MODE: %s", m_name.c_str(), GetErrno().c_str());
    return false;
  }

  value = 8;

  if (ioctl(m_fd, SPI_IOC_WR_BITS_PER_WORD, &value) == -1)
  {
    LogError("%s: SPI_IOC_WR_BITS_PER_WORD: %s", m_name.c_str(), GetErrno().c_str());
    return false;
  }

  if (ioctl(m_fd, SPI_IOC_RD_BITS_PER_WORD, &value) == -1)
  {
    LogError("%s: SPI_IOC_RD_BITS_PER_WORD: %s", m_name.c_str(), GetErrno().c_str());
    return false;
  }

  value = m_rate;

  if (ioctl(m_fd, SPI_IOC_WR_MAX_SPEED_HZ, &value) == -1)
  {
    LogError("%s: SPI_IOC_WR_MAX_SPEED_HZ: %s", m_name.c_str(), GetErrno().c_str());
    return false;
  }

  if (ioctl(m_fd, SPI_IOC_RD_MAX_SPEED_HZ, &value) == -1)
  {
    LogError("%s: SPI_IOC_RD_MAX_SPEED_HZ: %s", m_name.c_str(), GetErrno().c_str());
    return false;
  }

  if (m_type == LPD8806)
  {
    m_buffsize = (int)(((m_channels.size()/3)*2) + 4);
    m_buff = new uint8_t[m_buffsize];
  m_buff[0]=0x00;
	m_buff[1]=0x00;
	m_buff[2]=0x00;
	m_buff[3]=0x00;
    for (int i=4 ; i < m_buffsize ; i++) {
		if (i%2 == 0) {m_buff[i]=0x80;} else {m_buff[i]=0x00;}
	}
	m_max = 31.0f;
  }
  else if (m_type == WS2801)
  {
    m_buffsize = m_channels.size();
    m_buff = new uint8_t[m_buffsize];
    memset(m_buff, 0, m_buffsize);

    m_max = 255.0f;
  }

  //write out the buffer to turn off all leds
  if (!WriteBuffer())
    return false;

  return true;
}

bool CDeviceSPI::WriteOutput()
{
  //get the channel values from the clientshandler
  int64_t now = GetTimeUs();
  m_clients.FillChannels(m_channels, now, this);
  counter=4;
  
  
  
   for (int i = 0; i < (m_channels.size()-2); i+=3) {
      r=(uint16_t) ((m_channels[i].GetValue(now) * m_max)+0.5);
	  // r=Clamp(r,0,(uint16_t)(m_max));
	  g=(uint16_t) ((m_channels[i+1].GetValue(now) * m_max)+0.5);
	  // g=Clamp(g,0,(uint16_t)(m_max));
	  b=(uint16_t) ((m_channels[i+2].GetValue(now) * m_max)+0.5);
	  // b=Clamp(b,0,(uint16_t)(m_max));
	  d = (r*1024) + (g*32) + b + 32768;
	  m_buff[counter] =d >> 8;
      counter++;
      m_buff[counter] =d & 0x00FF;
      counter++;

  }
  if (!WriteBuffer())
    return false;

  m_timer.Wait();
  
  return true;
}

void CDeviceSPI::CloseDevice()
{
  if (m_fd != -1)
  {
    //turn off all leds
    
    m_buff[0]=0x00;
    m_buff[1]=0x00;
    m_buff[2]=0x00;
    m_buff[3]=0x00;

    for (int i=4 ; i < m_buffsize ; i++) {if (i%2==0) {m_buff[i]=0x80;} else {m_buff[i]=0x00;}}
    WriteBuffer();

    close(m_fd);
    m_fd = -1;
  }

  delete[] m_buff;
  m_buff = NULL;
  m_buffsize = 0;
}

bool CDeviceSPI::WriteBuffer()
{
  spi_ioc_transfer spi = {};
  spi.tx_buf = (__u64)m_buff;
  spi.len = m_buffsize;

  int returnv = ioctl(m_fd, SPI_IOC_MESSAGE(1), &spi);
  if (returnv == -1)
  {
    LogError("%s: %s %s", m_name.c_str(), m_output.c_str(), GetErrno().c_str());
    return false;
  }

  if (m_debug)
  {
    for (int i = 0; i < m_buffsize; i++)
      printf("%x ", m_buff[i]);

    printf("\n");
  }

  return true;
}
