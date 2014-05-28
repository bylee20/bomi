#ifndef UDF25_H
#define UDF25_H
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

#include <fstream>

namespace udf {

/**
 * The length of one Logical Block of a DVD.
 */
static constexpr int DVD_VIDEO_LB_LEN = 2048;

/**
 * Maximum length of filenames allowed in UDF.
 */
static constexpr int MAX_UDF_FILE_NAME_LEN = 2048;

struct Partition {
  int valid;
  char VolumeDesc[128];
  quint16 Flags;
  quint16 Number;
  char Contents[32];
  quint32 AccessType;
  quint32 Start;
  quint32 Length;
  quint32 Start_Correction;
};

struct AD {
  quint32 Location;
  quint32 Length;
  quint8  Flags;
  quint16 Partition;
};

/* Previously dvdread would assume files only had one AD chain, and since they
 * are 1GB or less, this is most problably true. However, now we handle chains
 * for large files. ECMA_167 does not specify the maximum number of chains, is
 * it as many as can fit in a 2048 block (minus ID 266 size), or some other
 * limit. For now, I have assumed that;
 * a 4.4GB file uses 5 AD chains. A BluRay disk can store 50GB of data, so the
 * largest file should be 50 GB. So the maximum number of chains should be
 * around 62.
 *
 * However, with AD chain extensions there has been examples of chains up to
 * around 1600 entries.
 */

#define UDF_MAX_AD_CHAINS 2000

struct FileAD {
    quint64 Length;
    quint32 num_AD;
    quint16 Partition;
    quint32 Partition_Start;
    quint32 Partition_Start_Correction;
    quint8  Type;
    quint16 Flags;
    struct AD AD_chain[UDF_MAX_AD_CHAINS];
};

struct extent_ad {
  quint32 location;
  quint32 length;
};

struct avdp_t {
  struct extent_ad mvds;
  struct extent_ad rvds;
};

struct pvd_t {
  quint8 VolumeIdentifier[32];
  quint8 VolumeSetIdentifier[128];
};

struct lbudf {
  quint32 lb;
  quint8 *data;
  /* needed for proper freeing */
  quint8 *data_base;
};

struct icbmap {
  quint32 lbn;
  struct FileAD  file;
};

struct udf_cache {
  int avdp_valid;
  struct avdp_t avdp;
  int pvd_valid;
  struct pvd_t pvd;
  int partition_valid;
  struct Partition partition;
  int rooticb_valid;
  struct AD rooticb;
  int lb_num;
  struct lbudf *lbs;
  int map_num;
  struct icbmap *maps;
};

enum UDFCacheType {
  PartitionCache, RootICBCache, LBUDFCache, MapCache, AVDPCache, PVDCache
};

/*
 * DVDReaddir entry types.
 */
enum udf_dir_type_t {
  DVD_DT_UNKNOWN = 0,
  DVD_DT_FIFO,
  DVD_DT_CHR,
  DVD_DT_DIR,
  DVD_DT_BLK,
  DVD_DT_REG,
  DVD_DT_LNK,
  DVD_DT_SOCK,
  DVD_DT_WHT
};

/*
 * DVDReaddir structure.
 * Extended a little from POSIX to also return filesize.
 */
struct udf_dirent_t {
  unsigned char  d_name[MAX_UDF_FILE_NAME_LEN];
  // "Shall not exceed 1023; Ecma-167 page 123"
  udf_dir_type_t d_type;       // DT_REG, DT_DIR
  unsigned int   d_namlen;
  quint64       d_filesize;
};


/*
 * DVDOpendir DIR* structure
 */
struct udf_dir_t {
  quint32 dir_location;
  quint32 dir_length;
  quint32 dir_current;   // Separate to _location should we one day want to
                          // implement dir_rewind()
  unsigned int current_p; // Internal implementation specific. UDFScanDirX
  udf_dirent_t entry;
};

class File;        class Dir;

class udf25
{

public:
  udf25( );
  ~udf25( );
  auto Open(const char *isofile) -> bool;
private:
  FileAD *UDFFindFile( const char* filename, quint64 *filesize );
  auto UDFScanDirX( udf_dir_t *dirp ) -> int;
  auto DVDUDFCacheLevel(int level) -> int;
  void *GetUDFCacheHandle();
  auto SetUDFCacheHandle(void *cache) -> void;
  auto GetUDFCache(UDFCacheType type,quint32 nr, void *data) -> int;
  auto UDFFindPartition( int partnum, struct Partition *part ) -> int;
  auto UDFGetAVDP( struct avdp_t *avdp) -> int;
  auto DVDReadLBUDF(quint32 lb_number, size_t block_count, unsigned char *data, int) -> int;
  auto ReadAt( int64_t pos, size_t len, unsigned char *data ) -> int;
  auto UDFMapICB( struct AD ICB, struct Partition *partition, struct FileAD *File ) -> int;
  auto UDFScanDir( struct FileAD Dir, char *FileName, struct Partition *partition, struct AD *FileICB, int cache_file_info) -> int;
  auto SetUDFCache(UDFCacheType type, quint32 nr, void *data) -> int;
protected:
  friend class File;
  friend class Dir;
    /* Filesystem cache */
  int m_udfcache_level; /* 0 - turned off, 1 - on */
  void *m_udfcache;
  std::fstream *m_fp;
};


class File {
public:
    File();
    ~File();
    File(udf25 *udf, const QString &fileName);
    auto isOpen() const -> bool { return m_file; }
    auto size() const -> quint64 { return m_size; }
    FileAD *handle() const { return m_file; }
    auto read(char *buffer, qint64 size) -> qint64;
    auto read(qint64 size) -> QByteArray;
    auto seek(int64_t lOffset, int whence) -> int64_t;
    auto fileName() const -> QString { return m_fileName; }
private:
    auto close() -> void;
    File(const File&) = delete;
    File &operator=(const File&) = delete;
    FileAD *m_file = nullptr;
    quint64 m_seek_pos = 0;  // in bytes
    quint64 m_size = 0;  // in bytes
    QString m_fileName;
    udf25 *m_udf = nullptr;
};

class Dir {
public:
    Dir();
    Dir(udf25 *udf, const QString &path);
    auto isOpen() const -> bool { return m_open; }
    auto path() const -> QString { return m_path; }
    auto files(bool withPath = true) const -> QStringList;
private:
    udf25 *m_udf = nullptr;
    bool m_open = false;
    QString m_path;
    QStringList m_files;
};

}

#endif
