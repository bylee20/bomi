/*
 *      Copyright (C) 2010 Team Boxee
 *      http://www.boxee.tv
 *
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *  Note: parts of this code comes from libdvdread.
 *  Jorgen Lundman and team boxee did the necessary modifications to support udf 2.5
 *
 */
#include "udf25.hpp"
#include "misc/log.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

DECLARE_LOG_CONTEXT(UDF)

/* For direct data access, LSB first */
#define GETN1(p) ((quint8)data[p])
#define GETN2(p) ((quint16)data[p] | ((quint16)data[(p) + 1] << 8))
#define GETN3(p) ((quint32)data[p] | ((quint32)data[(p) + 1] << 8)    \
                  | ((quint32)data[(p) + 2] << 16))
#define GETN4(p) ((quint32)data[p]                     \
                  | ((quint32)data[(p) + 1] << 8)      \
                  | ((quint32)data[(p) + 2] << 16)     \
                  | ((quint32)data[(p) + 3] << 24))
#define GETN8(p) ((quint64)data[p]                 \
                  | ((quint64)data[(p) + 1] << 8)  \
                  | ((quint64)data[(p) + 2] << 16) \
                  | ((quint64)data[(p) + 3] << 24) \
                  | ((quint64)data[(p) + 4] << 32) \
                  | ((quint64)data[(p) + 5] << 40) \
                  | ((quint64)data[(p) + 6] << 48) \
                  | ((quint64)data[(p) + 7] << 56))

/* This is wrong with regard to endianess */
#define GETN(p, n, target) memcpy(target, &data[p], n)

//using namespace XFILE;

namespace udf {

static int Unicodedecode( quint8 *data, int len, char *target )
{
  int p = 1, i = 0;

  if( ( data[ 0 ] == 8 ) || ( data[ 0 ] == 16 ) ) do {
    if( data[ 0 ] == 16 ) p++;  /* Ignore MSB of unicode16 */
    if( p < len ) {
      target[ i++ ] = data[ p++ ];
    }
  } while( p < len );

  target[ i ] = '\0';
  return 0;
}

static int UDFExtentAD( quint8 *data, quint32 *Length, quint32 *Location )
{
  *Length   = GETN4(0);
  *Location = GETN4(4);
  return 0;
}

static int UDFShortAD( quint8 *data, struct AD *ad )
{
  ad->Length = GETN4(0);
  ad->Flags = ad->Length >> 30;
  ad->Length &= 0x3FFFFFFF;
  ad->Location = GETN4(4);
  return 0;
}

static int UDFLongAD( quint8 *data, struct AD *ad )
{
  ad->Length = GETN4(0);
  ad->Flags = ad->Length >> 30;
  ad->Length &= 0x3FFFFFFF;
  ad->Location = GETN4(4);
  ad->Partition = GETN2(8);
  /* GETN(10, 6, Use); */
  return 0;
}

static int UDFExtAD( quint8 *data, struct AD *ad )
{
  ad->Length = GETN4(0);
  ad->Flags = ad->Length >> 30;
  ad->Length &= 0x3FFFFFFF;
  ad->Location = GETN4(12);
  ad->Partition = GETN2(16);
  /* GETN(10, 6, Use); */
  return 0;
}

static int UDFAD( quint8 *ptr, quint32 len, struct FileAD *fad)
{
  struct AD *ad;

  if (fad->num_AD  >= UDF_MAX_AD_CHAINS)
    return len;

  ad =  &fad->AD_chain[fad->num_AD];
  ad->Partition = fad->Partition;
  ad->Flags     = 0;
  fad->num_AD++;

  switch( fad->Flags & 0x0007 ) {
    case 0:
      UDFShortAD( ptr, ad );
      return 8;
    case 1:
      UDFLongAD( ptr, ad );
      return 16;
    case 2:
      UDFExtAD( ptr, ad );
      return 20;
    case 3:
      switch( len ) {
        case 8:
          UDFShortAD( ptr, ad );
          break;
        case 16:
          UDFLongAD( ptr,  ad );
          break;
        case 20:
          UDFExtAD( ptr, ad );
          break;
      }
      break;
    default:
      break;
  }

  return len;
}

static int UDFICB( quint8 *data, quint8 *FileType, quint16 *Flags )
{
  *FileType = GETN1(11);
  *Flags = GETN2(18);
  return 0;
}

static int UDFPartition( quint8 *data, quint16 *Flags, quint16 *Number,
                         char *Contents, quint32 *Start, quint32 *Length )
{
  *Flags = GETN2(20);
  *Number = GETN2(22);
  GETN(24, 32, Contents);
  *Start = GETN4(188);
  *Length = GETN4(192);
  return 0;
}

static int UDFLogVolume( quint8 *data, char *VolumeDescriptor )
{
  quint32 lbsize;
  Unicodedecode(&data[84], 128, VolumeDescriptor);
  lbsize = GETN4(212);  /* should be 2048 */
  if (lbsize != DVD_VIDEO_LB_LEN) return 1;
  return 0;
}

static int UDFAdEntry( quint8 *data, struct FileAD *fad )
{
  quint32 L_AD;
  unsigned int p;

  L_AD = GETN4(20);
  p = 24;
  while( p < 24 + L_AD ) {
    p += UDFAD( &data[ p ], L_AD, fad );
  }
  return 0;
}

static int UDFExtFileEntry( quint8 *data, struct FileAD *fad )
{
  quint32 L_EA, L_AD;
  unsigned int p;

  UDFICB( &data[ 16 ], &fad->Type, &fad->Flags );

  /* Init ad for an empty file (i.e. there isn't a AD, L_AD == 0 ) */
  fad->Length = GETN8(56); // 64-bit.

  L_EA = GETN4( 208);
  L_AD = GETN4( 212);
  p = 216 + L_EA;
  fad->num_AD = 0;
  while( p < 216 + L_EA + L_AD ) {
    p += UDFAD( &data[ p ], L_AD, fad );
  }
  return 0;

}

/* TagID 266
 * The differences in ExtFile are indicated below:
 * struct extfile_entry {
 *         struct desc_tag         tag;
 *         struct icb_tag          icbtag;
 *         quint32                uid;
 *         quint32                gid;
 *         quint32                perm;
 *         quint16                link_cnt;
 *         quint8                 rec_format;
 *         quint8                 rec_disp_attr;
 *         quint32                rec_len;
 *         quint64                inf_len;
 *         quint64                obj_size;        // NEW
 *         quint64                logblks_rec;
 *         struct timestamp        atime;
 *         struct timestamp        mtime;
 *         struct timestamp        ctime;           // NEW
 *         struct timestamp        attrtime;
 *         quint32                ckpoint;
 *         quint32                reserved1;       // NEW
 *         struct long_ad          ex_attr_icb;
 *         struct long_ad          streamdir_icb;   // NEW
 *         struct regid            imp_id;
 *         quint64                unique_id;
 *         quint32                l_ea;
 *         quint32                l_ad;
 *         quint8                 data[1];
 * } __packed;
 *
 * So old "l_ea" is now +216
 *
 * Lund
 */

static int UDFFileEntry( quint8 *data, struct FileAD *fad )
{
  quint32 L_EA, L_AD;
  unsigned int p;

  UDFICB( &data[ 16 ], &fad->Type, &fad->Flags );

  fad->Length = GETN8( 56 ); /* Was 4 bytes at 60, changed for 64bit. */

  L_EA = GETN4( 168 );
  L_AD = GETN4( 172 );

  if (176 + L_EA + L_AD > DVD_VIDEO_LB_LEN)
    return 0;

  p = 176 + L_EA;
  fad->num_AD = 0;
  while( p < 176 + L_EA + L_AD ) {
      p += UDFAD( &data[ p ], L_AD, fad );
  }
  return 0;
}

quint32 UDFFilePos(struct FileAD *File, quint64 pos, quint64 *res)
{
  quint32 i;

  for (i = 0; i < File->num_AD; i++) {

    if (pos < File->AD_chain[i].Length)
      break;

    pos -= File->AD_chain[i].Length;
  }

  if (i == File->num_AD)
    return 0;

  *res = (quint64)(File->Partition_Start + File->AD_chain[i].Location) * DVD_VIDEO_LB_LEN + pos;
  return File->AD_chain[i].Length - (quint32)pos;
}

quint32 UDFFileBlockPos(struct FileAD *File, quint32 lb)
{
  quint64 res;
  quint32 rem;
  rem = UDFFilePos(File, lb * DVD_VIDEO_LB_LEN, &res);
  if(rem > 0)
    return (quint32)(res / DVD_VIDEO_LB_LEN);
  else
    return 0;
}

static int UDFFileIdentifier( quint8 *data, quint8 *FileCharacteristics,
                              char *FileName, struct AD *FileICB )
{
  quint8 L_FI;
  quint16 L_IU;

  *FileCharacteristics = GETN1(18);
  L_FI = GETN1(19);
  UDFLongAD(&data[20], FileICB);
  L_IU = GETN2(36);
  if (L_FI) Unicodedecode(&data[38 + L_IU], L_FI, FileName);
  else FileName[0] = '\0';
  return 4 * ((38 + L_FI + L_IU + 3) / 4);
}

static int UDFDescriptor( quint8 *data, quint16 *TagID )
{
  *TagID = GETN2(0);
  /* TODO: check CRC 'n stuff */
  return 0;
}

auto udf25::UDFScanDirX( udf_dir_t *dirp ) -> int
{
  char filename[ MAX_UDF_FILE_NAME_LEN ];
  quint8 directory_base[ 2 * DVD_VIDEO_LB_LEN + 2048];
  quint8 *directory = (quint8 *)(((uintptr_t)directory_base & ~((uintptr_t)2047)) + 2048);
  quint32 lbnum;
  quint16 TagID;
  quint8 filechar;
  unsigned int p;
  struct AD FileICB;
  struct FileAD File;
  struct Partition partition;

  if(!(GetUDFCache(PartitionCache, 0, &partition))) {
    if(!UDFFindPartition(0, &partition))
      return 0;
  }

  /* Scan dir for ICB of file */
  lbnum = dirp->dir_current;

  // I have cut out the caching part of the original UDFScanDir() function
  // one day we should probably bring it back.
  memset(&File, 0, sizeof(File));

  if( DVDReadLBUDF( lbnum, 2, directory, 0 ) <= 0 ) {
    return 0;
  }


  p = dirp->current_p;
  while( p < dirp->dir_length ) {
    if( p > DVD_VIDEO_LB_LEN ) {
      ++lbnum;
      p -= DVD_VIDEO_LB_LEN;

      //Dir.Length -= DVD_VIDEO_LB_LEN;
      if (dirp->dir_length >= DVD_VIDEO_LB_LEN)
        dirp->dir_length -= DVD_VIDEO_LB_LEN;
      else
        dirp->dir_length = 0;

      if( DVDReadLBUDF( lbnum, 2, directory, 0 ) <= 0 ) {
        return 0;
      }
    }
    UDFDescriptor( &directory[ p ], &TagID );

    if( TagID == 257 ) {

      p += UDFFileIdentifier( &directory[ p ], &filechar,
                              filename, &FileICB );

      dirp->current_p = p;
      dirp->dir_current = lbnum;

      if (!*filename)  // No filename, simulate "." dirname
        strcpy((char *)dirp->entry.d_name, ".");
      else {
        // Bah, MSVC don't have strlcpy()
        strncpy((char *)dirp->entry.d_name, filename,
                sizeof(dirp->entry.d_name)-1);
        dirp->entry.d_name[ sizeof(dirp->entry.d_name) - 1 ] = 0;
      }


      // Look up the Filedata
      if( !UDFMapICB( FileICB, &partition, &File))
        return 0;
      if (File.Type == 4)
        dirp->entry.d_type = DVD_DT_DIR;
      else
        dirp->entry.d_type = DVD_DT_REG; // Add more types?

      dirp->entry.d_filesize = File.Length;

      return 1;

    } else {
      // Not TagID 257
      return 0;
    }
  }
  // End of DIR
  return 0;
}

auto udf25::SetUDFCache(UDFCacheType type, quint32 nr, void *data) -> int
{
  int n;
  struct udf_cache *c;
  void *tmp;

  if(DVDUDFCacheLevel(-1) <= 0)
    return 0;

  c = (struct udf_cache *)GetUDFCacheHandle();

  if(c == NULL) {
    c = (struct udf_cache *)calloc(1, sizeof(struct udf_cache));
    /* fprintf(stderr, "calloc: %d\n", sizeof(struct udf_cache)); */
    if(c == NULL)
      return 0;
    SetUDFCacheHandle(c);
  }


  switch(type) {
  case AVDPCache:
    c->avdp = *(struct avdp_t *)data;
    c->avdp_valid = 1;
    break;
  case PVDCache:
    c->pvd = *(struct pvd_t *)data;
    c->pvd_valid = 1;
    break;
  case PartitionCache:
    c->partition = *(struct Partition *)data;
    c->partition_valid = 1;
    break;
  case RootICBCache:
    c->rooticb = *(struct AD *)data;
    c->rooticb_valid = 1;
    break;
  case LBUDFCache:
    for(n = 0; n < c->lb_num; n++) {
      if(c->lbs[n].lb == nr) {
        /* replace with new data */
        c->lbs[n].data_base = ((quint8 **)data)[0];
        c->lbs[n].data = ((quint8 **)data)[1];
        c->lbs[n].lb = nr;
        return 1;
      }
    }
    c->lb_num++;
    tmp = realloc(c->lbs, c->lb_num * sizeof(struct lbudf));
    /*
    fprintf(stderr, "realloc lb: %d * %d = %d\n",
    c->lb_num, sizeof(struct lbudf),
    c->lb_num * sizeof(struct lbudf));
    */
    if(tmp == NULL) {
      if(c->lbs) free(c->lbs);
      c->lb_num = 0;
      return 0;
    }
    c->lbs = (struct lbudf *)tmp;
    c->lbs[n].data_base = ((quint8 **)data)[0];
    c->lbs[n].data = ((quint8 **)data)[1];
    c->lbs[n].lb = nr;
    break;
  case MapCache:
    for(n = 0; n < c->map_num; n++) {
      if(c->maps[n].lbn == nr) {
        /* replace with new data */
        c->maps[n] = *(struct icbmap *)data;
        c->maps[n].lbn = nr;
        return 1;
      }
    }
    c->map_num++;
    tmp = realloc(c->maps, c->map_num * sizeof(struct icbmap));
    /*
    fprintf(stderr, "realloc maps: %d * %d = %d\n",
      c->map_num, sizeof(struct icbmap),
      c->map_num * sizeof(struct icbmap));
    */
    if(tmp == NULL) {
      if(c->maps) free(c->maps);
      c->map_num = 0;
      return 0;
    }
    c->maps = (struct icbmap *)tmp;
    c->maps[n] = *(struct icbmap *)data;
    c->maps[n].lbn = nr;
    break;
  default:
    return 0;
  }

  return 1;
}

auto udf25::DVDUDFCacheLevel(int level) -> int
{
  if(level > 0) {
    level = 1;
  } else if(level < 0) {
    return m_udfcache_level;
  }

  m_udfcache_level = level;

  return level;
}

auto udf25::GetUDFCacheHandle() -> void*
{
  return m_udfcache;
}

auto udf25::SetUDFCacheHandle(void *cache) -> void
{
  m_udfcache = cache;
}

auto udf25::GetUDFCache(UDFCacheType type, quint32 nr, void *data) -> int
{
  int n;
  struct udf_cache *c;

  if(DVDUDFCacheLevel(-1) <= 0)
    return 0;

  c = (struct udf_cache *)GetUDFCacheHandle();

  if(c == NULL)
    return 0;

  switch(type) {
  case AVDPCache:
    if(c->avdp_valid) {
      *(struct avdp_t *)data = c->avdp;
      return 1;
    }
    break;
  case PVDCache:
    if(c->pvd_valid) {
      *(struct pvd_t *)data = c->pvd;
      return 1;
    }
    break;
  case PartitionCache:
    if(c->partition_valid) {
      *(struct Partition *)data = c->partition;
      return 1;
    }
    break;
  case RootICBCache:
    if(c->rooticb_valid) {
      *(struct AD *)data = c->rooticb;
      return 1;
    }
    break;
  case LBUDFCache:
    for(n = 0; n < c->lb_num; n++) {
      if(c->lbs[n].lb == nr) {
        *(quint8 **)data = c->lbs[n].data;
        return 1;
      }
    }
    break;
  case MapCache:
    for(n = 0; n < c->map_num; n++) {
      if(c->maps[n].lbn == nr) {
        *(struct icbmap *)data = c->maps[n];
        return 1;
      }
    }
    break;
  default:
    break;
  }

  return 0;
}

auto udf25::ReadAt( int64_t pos, size_t len, unsigned char *data ) -> int
{
  if (!m_fp->seekg(pos))
    return -1;
  if (!m_fp->read((char*)data, len))
    _Error("ReadFile - less data than requested available!");
  return m_fp->gcount();
}

auto udf25::DVDReadLBUDF( quint32 lb_number, size_t block_count, unsigned char *data, int /*encrypted*/ ) -> int
{
  int ret;
  size_t  len = block_count * DVD_VIDEO_LB_LEN;
  int64_t pos = lb_number   * (int64_t)DVD_VIDEO_LB_LEN;

  ret = ReadAt(pos, len, data);
  if(ret < 0)
    return ret;

  if((unsigned int)ret < len)
  {
      _Error("DVDReadLBUDF -  Block was not complete, setting to wanted %% (got %%)", (unsigned int)len, (unsigned int)ret);
    memset(&data[ret], 0, len - ret);
  }

  return len / DVD_VIDEO_LB_LEN;
}

auto udf25::UDFGetAVDP( struct avdp_t *avdp) -> int
{
  quint8 Anchor_base[ DVD_VIDEO_LB_LEN + 2048 ];
  quint8 *Anchor = (quint8 *)(((uintptr_t)Anchor_base & ~((uintptr_t)2047)) + 2048);
  quint32 lbnum, MVDS_location, MVDS_length;
  quint16 TagID;
  quint32 lastsector;
  int terminate;
  struct avdp_t;

  if(GetUDFCache(AVDPCache, 0, avdp))
    return 1;

  /* Find Anchor */
  lastsector = 0;
  lbnum = 256;   /* Try #1, prime anchor */
  terminate = 0;

  for(;;) {
    if( DVDReadLBUDF( lbnum, 1, Anchor, 0 ) > 0 ) {
      UDFDescriptor( Anchor, &TagID );
    } else {
      TagID = 0;
    }
    if (TagID != 2) {
      /* Not an anchor */
      if( terminate ) return 0; /* Final try failed */

      if( lastsector ) {
        /* We already found the last sector.  Try #3, alternative
         * backup anchor.  If that fails, don't try again.
         */
        lbnum = lastsector;
        terminate = 1;
      } else {
        /* TODO: Find last sector of the disc (this is optional). */
        if( lastsector )
          /* Try #2, backup anchor */
          lbnum = lastsector - 256;
        else
          /* Unable to find last sector */
          return 0;
      }
    } else
      /* It's an anchor! We can leave */
      break;
  }
  /* Main volume descriptor */
  UDFExtentAD( &Anchor[ 16 ], &MVDS_length, &MVDS_location );
  avdp->mvds.location = MVDS_location;
  avdp->mvds.length = MVDS_length;

  /* Backup volume descriptor */
  UDFExtentAD( &Anchor[ 24 ], &MVDS_length, &MVDS_location );
  avdp->rvds.location = MVDS_location;
  avdp->rvds.length = MVDS_length;

  SetUDFCache(AVDPCache, 0, avdp);

  return 1;
}

auto udf25::UDFFindPartition( int partnum, struct Partition *part ) -> int
{
  quint8 LogBlock_base[ DVD_VIDEO_LB_LEN + 2048 ];
  quint8 *LogBlock = (quint8 *)(((uintptr_t)LogBlock_base & ~((uintptr_t)2047)) + 2048);
  quint32 lbnum, MVDS_location, MVDS_length;
  quint16 TagID;
  int i, volvalid;
  struct avdp_t avdp;

  if(!UDFGetAVDP(&avdp))
    return 0;

  /* Main volume descriptor */
  MVDS_location = avdp.mvds.location;
  MVDS_length = avdp.mvds.length;

  part->valid = 0;
  volvalid = 0;
  part->VolumeDesc[ 0 ] = '\0';
  i = 1;
  do {
    /* Find Volume Descriptor */
    lbnum = MVDS_location;
    do {

      if( DVDReadLBUDF( lbnum++, 1, LogBlock, 0 ) <= 0 )
        TagID = 0;
      else
        UDFDescriptor( LogBlock, &TagID );

      if( ( TagID == 5 ) && ( !part->valid ) ) {
        /* Partition Descriptor */
        UDFPartition( LogBlock, &part->Flags, &part->Number,
                      part->Contents, &part->Start, &part->Length );
        part->Start_Correction = 0;
        part->valid = ( partnum == part->Number );
      } else if( ( TagID == 6 ) && ( !volvalid ) ) {
        /* Logical Volume Descriptor */
        if( UDFLogVolume( LogBlock, part->VolumeDesc ) ) {
          /* TODO: sector size wrong! */
        } else
          volvalid = 1;
      }

    } while( ( lbnum <= MVDS_location + ( MVDS_length - 1 )
               / DVD_VIDEO_LB_LEN ) && ( TagID != 8 )
             && ( ( !part->valid ) || ( !volvalid ) ) );

    if( ( !part->valid) || ( !volvalid ) ) {
      /* Backup volume descriptor */
      MVDS_location = avdp.mvds.location;
      MVDS_length = avdp.mvds.length;
    }
  } while( i-- && ( ( !part->valid ) || ( !volvalid ) ) );

  /* Look for a metadata partition */
  lbnum = part->Start;
  do {
    if( DVDReadLBUDF( lbnum++, 1, LogBlock, 0 ) <= 0 )
      TagID = 0;
    else
      UDFDescriptor( LogBlock, &TagID );

    /*
     * It seems that someone added a FileType of 250, which seems to be
     * a "redirect" style file entry. If we discover such an entry, we
     * add on the "location" to partition->Start, and try again.
     * Who added this? Is there any official guide somewhere?
     * 2008/09/17 lundman
     * Should we handle 261(250) as well? FileEntry+redirect
     */
    if( TagID == 266 ) {
      struct FileAD File;
      File.Partition       = part->Number;
      File.Partition_Start = part->Start;

      UDFExtFileEntry( LogBlock, &File );
      if (File.Type == 250) {
        part->Start  += File.AD_chain[0].Location;
        // we need to remember this correction because read positions are relative to the non-indirected partition start
        part->Start_Correction = File.AD_chain[0].Location;
        part->Length  = File.AD_chain[0].Length;
        break;
      }
    }

  } while( ( lbnum < part->Start + part->Length )
          && ( TagID != 8 ) && ( TagID != 256 ) );

  /* We only care for the partition, not the volume */
  return part->valid;
}

auto udf25::UDFMapICB( struct AD ICB, struct Partition *partition, struct FileAD *File ) -> int
{
  quint8 LogBlock_base[DVD_VIDEO_LB_LEN + 2048];
  quint8 *LogBlock = (quint8 *)(((uintptr_t)LogBlock_base & ~((uintptr_t)2047)) + 2048);
  quint32 lbnum;
  quint16 TagID;
  struct icbmap tmpmap;

  lbnum = partition->Start + ICB.Location;
  tmpmap.lbn = lbnum;
  if(GetUDFCache(MapCache, lbnum, &tmpmap)) {
    memcpy(File, &tmpmap.file, sizeof(tmpmap.file));
    return 1;
  }

  memset(File, 0, sizeof(*File));
  File->Partition       = partition->Number;
  File->Partition_Start = partition->Start;
  File->Partition_Start_Correction = partition->Start_Correction;

  do {
    if( DVDReadLBUDF( lbnum++, 1, LogBlock, 0 ) <= 0 )
      TagID = 0;
    else
      UDFDescriptor( LogBlock, &TagID );

    if( TagID == 261 ) {
      UDFFileEntry( LogBlock, File );
      break;
    };

    /* ExtendedFileInfo */
    if( TagID == 266 ) {
      UDFExtFileEntry( LogBlock, File );
      break;
    }


  } while( lbnum <= partition->Start + ICB.Location + ( ICB.Length - 1 )
             / DVD_VIDEO_LB_LEN );


  if(File->Type) {

    /* check if ad chain continues elsewhere */
    while(File->num_AD && File->AD_chain[File->num_AD-1].Flags == 3) {
      struct AD* ad = &File->AD_chain[File->num_AD-1];

      /* remove the forward pointer from the list */
      File->num_AD--;

      if( DVDReadLBUDF( File->Partition_Start + ad->Location, 1, LogBlock, 0 ) <= 0 )
        TagID = 0;
      else
        UDFDescriptor( LogBlock, &TagID );

      if( TagID == 258 ) {
        /* add all additional entries */
        UDFAdEntry( LogBlock, File );
      } else {
        return 0;
      }
    }

    memcpy(&tmpmap.file, File, sizeof(tmpmap.file));
    SetUDFCache(MapCache, tmpmap.lbn, &tmpmap);

    return 1;
  }

  return 0;
}

auto udf25::UDFScanDir( struct FileAD Dir, char *FileName, struct Partition *partition, struct AD *FileICB, int cache_file_info) -> int
{
  char filename[ MAX_UDF_FILE_NAME_LEN ];
  quint8 directory_base[ 2 * DVD_VIDEO_LB_LEN + 2048];
  quint8 *directory = (quint8 *)(((uintptr_t)directory_base & ~((uintptr_t)2047)) + 2048);
  quint32 lbnum;
  quint16 TagID;
  quint8 filechar;
  unsigned int p;
  quint8 *cached_dir_base = NULL, *cached_dir;
  quint32 dir_lba;
  struct AD tmpICB;
  int found = 0;
  int in_cache = 0;

  /* Scan dir for ICB of file */
  lbnum = partition->Start + Dir.AD_chain[0].Location;

  if(DVDUDFCacheLevel(-1) > 0) {
    /* caching */

    if(!GetUDFCache(LBUDFCache, lbnum, &cached_dir)) {
      dir_lba = (Dir.AD_chain[0].Length + DVD_VIDEO_LB_LEN) / DVD_VIDEO_LB_LEN;
      if((cached_dir_base = (quint8 *)malloc(dir_lba * DVD_VIDEO_LB_LEN + 2048)) == NULL)
        return 0;
      cached_dir = (quint8 *)(((uintptr_t)cached_dir_base & ~((uintptr_t)2047)) + 2048);
      if( DVDReadLBUDF( lbnum, dir_lba, cached_dir, 0) <= 0 ) {
        free(cached_dir_base);
        cached_dir_base = NULL;
        cached_dir = NULL;
      }
      /*
      if(cached_dir) {
        fprintf(stderr, "malloc dir: %d\n",  dir_lba * DVD_VIDEO_LB_LEN);
      }
      */
      {
        quint8 *data[2];
        data[0] = cached_dir_base;
        data[1] = cached_dir;
        SetUDFCache(LBUDFCache, lbnum, data);
      }
    } else
      in_cache = 1;

    if(cached_dir == NULL)
      return 0;

    p = 0;

    while( p < Dir.AD_chain[0].Length ) {  /* Assuming dirs don't use chains? */
      UDFDescriptor( &cached_dir[ p ], &TagID );
      if( TagID == 257 ) {
        p += UDFFileIdentifier( &cached_dir[ p ], &filechar,
                                filename, &tmpICB );
        if(cache_file_info && !in_cache) {
          struct FileAD tmpFile;

          memset(&tmpFile, 0, sizeof(tmpFile));

          if( !strcasecmp( FileName, filename ) ) {
            memcpy(FileICB, &tmpICB, sizeof(tmpICB));
            found = 1;
          }
          UDFMapICB(tmpICB, partition, &tmpFile);
        } else {
          if( !strcasecmp( FileName, filename ) ) {
            memcpy(FileICB, &tmpICB, sizeof(tmpICB));
            return 1;
          }
        }
      } else {
        if(cache_file_info && (!in_cache) && found)
          return 1;
        return 0;
      }
    }
    if(cache_file_info && (!in_cache) && found)
      return 1;
    return 0;
  }

  if( DVDReadLBUDF( lbnum, 2, directory, 0 ) <= 0 )
    return 0;

  p = 0;
  while( p < Dir.AD_chain[0].Length ) {
    if( p > DVD_VIDEO_LB_LEN ) {
      ++lbnum;
      p -= DVD_VIDEO_LB_LEN;
      Dir.AD_chain[0].Length -= DVD_VIDEO_LB_LEN;
      if( DVDReadLBUDF( lbnum, 2, directory, 0 ) <= 0 ) {
        return 0;
      }
    }
    UDFDescriptor( &directory[ p ], &TagID );
    if( TagID == 257 ) {
      p += UDFFileIdentifier( &directory[ p ], &filechar,
                              filename, FileICB );
      if( !strcasecmp( FileName, filename ) ) {
        return 1;
      }
    } else
      return 0;
  }

  return 0;
}

udf25::udf25( )
{
  m_fp = NULL;
  m_udfcache_level = 1;
  m_udfcache = NULL;
}

udf25::~udf25( )
{
  delete m_fp;
  free(m_udfcache);
}

auto udf25::UDFFindFile( const char* filename, quint64 *filesize ) -> FileAD*
{
  quint8 LogBlock_base[ DVD_VIDEO_LB_LEN + 2048 ];
  quint8 *LogBlock = (quint8 *)(((uintptr_t)LogBlock_base & ~((uintptr_t)2047)) + 2048);
  quint32 lbnum;
  quint16 TagID;
  struct Partition partition;
  struct AD RootICB, ICB;
  struct FileAD File;
  char tokenline[ MAX_UDF_FILE_NAME_LEN ];
  char *token;
  struct FileAD *result;

  *filesize = 0;
  tokenline[0] = '\0';
  strncat(tokenline, filename, MAX_UDF_FILE_NAME_LEN - 1);
  memset(&ICB, 0, sizeof(ICB));
  memset(&File, 0, sizeof(File));

  if(!(GetUDFCache(PartitionCache, 0, &partition) &&
       GetUDFCache(RootICBCache, 0, &RootICB))) {
    /* Find partition, 0 is the standard location for DVD Video.*/
    if( !UDFFindPartition(0, &partition ) ) return 0;
    SetUDFCache(PartitionCache, 0, &partition);

    /* Find root dir ICB */
    lbnum = partition.Start;
    do {
      if( DVDReadLBUDF( lbnum++, 1, LogBlock, 0 ) <= 0 )
        TagID = 0;
      else
        UDFDescriptor( LogBlock, &TagID );

      /* File Set Descriptor */
      if( TagID == 256 )  /* File Set Descriptor */
        UDFLongAD( &LogBlock[ 400 ], &RootICB );
    } while( ( lbnum < partition.Start + partition.Length )
             && ( TagID != 8 ) && ( TagID != 256 ) );

    /* Sanity checks. */
    if( TagID != 256 )
      return NULL;
    /* This following test will fail under UDF2.50 images, as it is no longer
     * valid */
    /*if( RootICB.Partition != 0 )
      return 0;*/
    SetUDFCache(RootICBCache, 0, &RootICB);
  }

  /* Find root dir */
  if( !UDFMapICB( RootICB, &partition, &File ) )
    return NULL;
  if( File.Type != 4 )
    return NULL;  /* Root dir should be dir */
  {
    int cache_file_info = 0;
    /* Tokenize filepath */
    token = strtok(tokenline, "/");

    while( token != NULL ) {
      if( !UDFScanDir( File, token, &partition, &ICB,
                       cache_file_info))
        return NULL;
      if( !UDFMapICB( ICB, &partition, &File ) )
        return NULL;
      if(!strcmp(token, "index.bdmv"))
        cache_file_info = 1;
      token = strtok( NULL, "/" );
    }
  }

  /* Sanity check. */
  if( File.AD_chain[0].Partition != 0 )
    return 0;
  *filesize = File.Length;
  /* Hack to not return partition.Start for empty files. */
  if( !File.AD_chain[0].Location )
    return NULL;

  /* Allocate a new UDF_FILE and return it. */
  result = (struct FileAD *) malloc(sizeof(*result));
  if (!result) return NULL;

  memcpy(result, &File, sizeof(*result));
  return result;
}

auto udf25::Open(const char *isofile) -> bool
{
  m_fp = new std::fstream(isofile, std::ios::binary | std::ios::in);

  if(!m_fp->is_open())
  {
    _Error("file_open - Could not open input");
    delete m_fp;
    m_fp = NULL;
    return false;
  }
  return true;
}



//int64_t udf25::GetFileSize(HANDLE hFile)
//{
//  BD_FILE bdfile = (BD_FILE)hFile;

//  if(bdfile == NULL)
//    return -1;

//  return bdfile->filesize;
//}

//int64_t udf25::GetFilePosition(HANDLE hFile)
//{
//  BD_FILE bdfile = (BD_FILE)hFile;

//  if(bdfile == NULL)
//    return -1;

//  return bdfile->seek_pos;
//}




File::File(udf25 *udf, const QString &fileName): m_udf(udf) {
    if (udf->m_fp)
        m_file = udf->UDFFindFile(fileName.toLocal8Bit(), &m_size);
}

File::~File() {
    close();
}

auto File::read(qint64 size) -> QByteArray
{
    QByteArray data;
    data.resize(size);
    data.resize(read(data.data(), size));
    return data;
}

auto File::read(char *buffer, qint64 size) -> qint64
{
    /* Check arguments. */
    if(!isOpen() || !buffer)
        return -1;
    qint64 len_origin = size;
    quint64 pos;
    int      ret;
    while(size > 0) {
        auto len = UDFFilePos(m_file, m_seek_pos, &pos);
        if(!len)
            break;
        // correct for partition indirection if applicable
        pos -= m_file->Partition_Start_Correction * DVD_VIDEO_LB_LEN;

        if((quint32)size < len)
            len = size;

        ret = m_udf->ReadAt(pos, len, (uchar*)buffer);
        if(ret < 0) {
            _Error("ReadFile - error during read" );
            return ret;
        }

        if(ret == 0)
            break;

        m_seek_pos += ret;
        buffer     += ret;
        size       -= ret;
    }
    return len_origin - size;
}

auto File::close() -> void
{
    if (m_file)
        free(m_file);
    m_file = nullptr;
    m_size = 0;
    m_seek_pos = 0;
}

auto File::seek(int64_t offset, int whence) -> int64_t
{
    if (!isOpen())
        return -1;
    int64_t seek_pos = m_seek_pos;
    switch (whence) {
    case SEEK_SET:
        // cur = pos
        m_seek_pos = offset;
        break;

    case SEEK_CUR:
        // cur += pos
        m_seek_pos += offset;
        break;
    case SEEK_END:
        // end += pos
        m_seek_pos = m_size + offset;
        break;
    }
    if (m_seek_pos > m_size)
        m_seek_pos = seek_pos;
    return m_seek_pos;
}

Dir::Dir(udf25 *udf, const QString &path): m_udf(udf), m_path(path) {
    File file(udf, path);
    if (!file.isOpen())
      return;
    udf_dir_t dir; memset(&dir, 0, sizeof(dir));
    dir.dir_location = UDFFileBlockPos(file.handle(), 0);
    dir.dir_current = UDFFileBlockPos(file.handle(), 0);
    dir.dir_length = file.size();
    m_open = true;

    auto read = [this, &dir] () -> QString {
        if (!m_udf->UDFScanDirX(&dir)) {
            dir.current_p = 9;
            dir.dir_current = dir.dir_location;
            return QString();
        } else
            return QString::fromLocal8Bit((char*)dir.entry.d_name);
    };
    for (;;) {
        auto file = read();
        if (file.isEmpty())
            break;
        if (_IsOneOf(file, "."_a, ".."_a) || dir.entry.d_type == DVD_DT_DIR)
            continue;
        m_files.append(file);
    }
}

auto Dir::files(bool withPath) const -> QStringList
{
    if (!m_open)
        return QStringList();
    if (!withPath)
        return m_files;
    QStringList files; files.reserve(m_files.size());
    for (auto &file : m_files)
        files.append(m_path % '/'_q % file);
    return files;
}

}
