struct io_request{
    struct io_request *next;
    unsigned short type;
    unsigned char proc_id;
};
