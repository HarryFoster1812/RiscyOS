typedef struct {
  unsigned int fd;
  unsigned int current_cluster;
  unsigned int current_sector;
  unsigned int current_byte;
  // other fields would be permissions but in this system it is read only currently
}FILE;
