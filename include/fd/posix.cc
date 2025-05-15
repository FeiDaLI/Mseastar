// #include "posix.hh"
// #include <string>
// #include <vector>
// #include <fcntl.h>

// void mmap_deleter::operator()(void* ptr) const {
//     ::munmap(ptr, _size);
// }

// mmap_area mmap_anonymous(void* addr, size_t length, int prot, int flags) {
//     void* ret = ::mmap(addr, length, prot, flags | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
//     throw_system_error_on(ret == MAP_FAILED, "mmap");
//     return mmap_area(reinterpret_cast<char*>(ret), mmap_deleter{length});
// }

// file_desc file_desc::temporary(const std::string& directory) {
//     std::string path = directory + "/XXXXXX";
//     std::vector<char> pathname(path.begin(), path.end());
//     pathname.push_back('\0');
//     int fd = ::mkstemp(pathname.data());
//     throw_system_error_on(fd == -1, "mkstemp");
//     ::unlink(pathname.data()); // Delete when closed
//     return file_desc(fd);
// } 