/*
 * vim: softtabstop=4 shiftwidth=4 cindent foldmethod=marker expandtab
 *
 * $LastChangedDate$
 * $Revision$
 * $LastChangedBy$
 * $URL$
 *
 * Copyright 2009-2011 Eric Connell
 * Copyright 2010-2011 Roman Tetelman
 *
 * This file is part of Mangler.
 *
 * Mangler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mangler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mangler.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _MANGLER_OSS_H
#define _MANGLER_OSS_H

class ManglerOSS : public ManglerBackend {
    int             oss_fd;
    uint32_t        pcm_framesize;
public:
    virtual bool            open(int type, Glib::ustring device, int rate, int channels);
    virtual void            close(bool drain = false);
    virtual bool            write(uint8_t *sample, uint32_t length, int channels);
    virtual bool            read(uint8_t* buf);
    virtual Glib::ustring   getAudioSubsystem(void);
    ManglerOSS(uint32_t rate, uint8_t channels, uint32_t pcm_framesize);
    virtual ~ManglerOSS();

    static void             getDeviceList(std::vector<ManglerAudioDevice*>& inputDevices, std::vector<ManglerAudioDevice*>& outputDevices);
};

#endif

