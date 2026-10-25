/* sd2iec - SD/MMC to Commodore serial bus interface/controller
   Copyright (C) 2007-2026  Ingo Korb <ingo@akana.de>

   Inspired by MMC2IEC by Lars Pontoppidan et al.

   FAT filesystem access based on code from ChaN, see fatfs/ff.c|h.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License only.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


   ff.h: FatFs API with the calling conventions sd2iec uses

   The FatFs functions that take a path have no volume argument, the macros
   below select the volume first. The current directory is FATFS.cdir, the
   start cluster of the directory (0 is the root).
*/

#ifndef SD2IEC_FF_H
#define SD2IEC_FF_H

#include "integer.h"
#include "fatfs/ff.h"

/* Longest long name that is created or returned in FILINFO.lfn */
#define _MAX_LFN_LENGTH FF_MAX_LFN

#define FR_RW_ERROR FR_DISK_ERR

/* Calls with a volume argument */
#define f_open(fs,fp,path,mode)      (l_select(fs), (f_open)(fp, (const TCHAR *)(path), mode))
#define f_stat(fs,path,fno)          (l_select(fs), (f_stat)((const TCHAR *)(path), fno))
#define f_unlink(fs,path)            (l_select(fs), (f_unlink)((const TCHAR *)(path)))
#define f_mkdir(fs,path)             (l_select(fs), (f_mkdir)((const TCHAR *)(path)))
#define f_chmod(fs,path,attr,mask)   (l_select(fs), (f_chmod)((const TCHAR *)(path), attr, mask))
#define f_rename(fs,old,new)         (l_select(fs), (f_rename)((const TCHAR *)(old), (const TCHAR *)(new)))
#define f_mount(drv,fs)              l_mount(drv, fs)
#define f_readdir(dp,fno)            l_readdir(dp, fno)

#endif
