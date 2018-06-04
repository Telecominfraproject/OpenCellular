struct syscall_result {
	long val;
	int errno;
};

static struct syscall_result syscall_return(long result)
{
	struct syscall_result res;
	if (((unsigned long)result) >= ((unsigned long)-125)) {
		res.errno = - result;
		res.val = -1;
	} else {
		res.errno = 0;
		res.val = result;
	}
	return res;
}

static struct syscall_result syscall0(unsigned long nr)
{
	long res;
	asm volatile(
		"int $0x80"
		: "=a" (res)
		: "a" (nr));
	return syscall_return(res);
}

static struct syscall_result syscall1(unsigned long nr, unsigned long arg1)
{
	long res;
	asm volatile(
		"int $0x80"
		: "=a" (res)
		: "a" (nr), "b" (arg1));
	return syscall_return(res);

}

static struct syscall_result syscall2(unsigned long nr, unsigned long arg1, unsigned long arg2)
{
	long res;
	asm volatile(
		"int $0x80"
		: "=a" (res)
		: "a" (nr), "b" (arg1), "c" (arg2));
	return syscall_return(res);

}


static struct syscall_result syscall3(unsigned long nr, unsigned long arg1, unsigned long arg2,
	unsigned long arg3)
{
	long res;
	asm volatile(
		"int $0x80"
		: "=a" (res)
		: "a" (nr), "b" (arg1), "c" (arg2), "d" (arg3));
	return syscall_return(res);

}

static struct syscall_result syscall4(unsigned long nr, unsigned long arg1, unsigned long arg2,
	unsigned long arg3, unsigned long arg4)
{
	long res;
	asm volatile(
		"int $0x80"
		: "=a" (res)
		: "a" (nr), "b" (arg1), "c" (arg2), "d" (arg3), "S" (arg4));
	return syscall_return(res);

}

static struct syscall_result syscall5(unsigned long nr, unsigned long arg1, unsigned long arg2,
	unsigned long arg3, unsigned long arg4, unsigned long arg5)
{
	long res;
	asm volatile(
		"int $0x80"
		: "=a" (res)
		: "a" (nr), "b" (arg1), "c" (arg2), "d" (arg3),
		"S" (arg4), "D" (arg5));
	return syscall_return(res);

}

#define NR_exit                 1
#define NR_fork                 2
#define NR_read                 3
#define NR_write                4
#define NR_open                 5
#define NR_close                6
#define NR_waitpid              7
#define NR_creat                8
#define NR_link                 9
#define NR_unlink              10
#define NR_execve              11
#define NR_chdir               12
#define NR_time                13
#define NR_mknod               14
#define NR_chmod               15
#define NR_lchown              16
#define NR_break               17
#define NR_oldstat             18
#define NR_lseek               19
#define NR_getpid              20
#define NR_mount               21
#define NR_umount              22
#define NR_setuid              23
#define NR_getuid              24
#define NR_stime               25
#define NR_ptrace              26
#define NR_alarm               27
#define NR_oldfstat            28
#define NR_pause               29
#define NR_utime               30
#define NR_stty                31
#define NR_gtty                32
#define NR_access              33
#define NR_nice                34
#define NR_ftime               35
#define NR_sync                36
#define NR_kill                37
#define NR_rename              38
#define NR_mkdir               39
#define NR_rmdir               40
#define NR_dup                 41
#define NR_pipe                42
#define NR_times               43
#define NR_prof                44
#define NR_brk                 45
#define NR_setgid              46
#define NR_getgid              47
#define NR_signal              48
#define NR_geteuid             49
#define NR_getegid             50
#define NR_acct                51
#define NR_umount2             52
#define NR_lock                53
#define NR_ioctl               54
#define NR_fcntl               55
#define NR_mpx                 56
#define NR_setpgid             57
#define NR_ulimit              58
#define NR_oldolduname         59
#define NR_umask               60
#define NR_chroot              61
#define NR_ustat               62
#define NR_dup2                63
#define NR_getppid             64
#define NR_getpgrp             65
#define NR_setsid              66
#define NR_sigaction           67
#define NR_sgetmask            68
#define NR_ssetmask            69
#define NR_setreuid            70
#define NR_setregid            71
#define NR_sigsuspend          72
#define NR_sigpending          73
#define NR_sethostname         74
#define NR_setrlimit           75
#define NR_getrlimit           76
#define NR_getrusage           77
#define NR_gettimeofday        78
#define NR_settimeofday        79
#define NR_getgroups           80
#define NR_setgroups           81
#define NR_select              82
#define NR_symlink             83
#define NR_oldlstat            84
#define NR_readlink            85
#define NR_uselib              86
#define NR_swapon              87
#define NR_reboot              88
#define NR_readdir             89
#define NR_mmap                90
#define NR_munmap              91
#define NR_truncate            92
#define NR_ftruncate           93
#define NR_fchmod              94
#define NR_fchown              95
#define NR_getpriority         96
#define NR_setpriority         97
#define NR_profil              98
#define NR_statfs              99
#define NR_fstatfs            100
#define NR_ioperm             101
#define NR_socketcall         102
#define NR_syslog             103
#define NR_setitimer          104
#define NR_getitimer          105
#define NR_stat               106
#define NR_lstat              107
#define NR_fstat              108
#define NR_olduname           109
#define NR_iopl               110
#define NR_vhangup            111
#define NR_idle               112
#define NR_vm86old            113
#define NR_wait4              114
#define NR_swapoff            115
#define NR_sysinfo            116
#define NR_ipc                117
#define NR_fsync              118
#define NR_sigreturn          119
#define NR_clone              120
#define NR_setdomainname      121
#define NR_uname              122
#define NR_modify_ldt         123
#define NR_adjtimex           124
#define NR_mprotect           125
#define NR_sigprocmask        126
#define NR_create_module      127
#define NR_init_module        128
#define NR_delete_module      129
#define NR_get_kernel_syms    130
#define NR_quotactl           131
#define NR_getpgid            132
#define NR_fchdir             133
#define NR_bdflush            134
#define NR_sysfs              135
#define NR_personality        136
#define NR_afs_syscall        137 /* Syscall for Andrew File System */
#define NR_setfsuid           138
#define NR_setfsgid           139
#define NR__llseek            140
#define NR_getdents           141
#define NR__newselect         142
#define NR_flock              143
#define NR_msync              144
#define NR_readv              145
#define NR_writev             146
#define NR_getsid             147
#define NR_fdatasync          148
#define NR__sysctl            149
#define NR_mlock              150
#define NR_munlock            151
#define NR_mlockall           152
#define NR_munlockall         153
#define NR_sched_setparam             154
#define NR_sched_getparam             155
#define NR_sched_setscheduler         156
#define NR_sched_getscheduler         157
#define NR_sched_yield                158
#define NR_sched_get_priority_max     159
#define NR_sched_get_priority_min     160
#define NR_sched_rr_get_interval      161
#define NR_nanosleep          162
#define NR_mremap             163
#define NR_setresuid          164
#define NR_getresuid          165
#define NR_vm86               166
#define NR_query_module       167
#define NR_poll               168
#define NR_nfsservctl         169
#define NR_setresgid          170
#define NR_getresgid          171
#define NR_prctl              172
#define NR_rt_sigreturn       173
#define NR_rt_sigaction       174
#define NR_rt_sigprocmask     175
#define NR_rt_sigpending      176
#define NR_rt_sigtimedwait    177
#define NR_rt_sigqueueinfo    178
#define NR_rt_sigsuspend      179
#define NR_pread              180
#define NR_pwrite             181
#define NR_chown              182
#define NR_getcwd             183
#define NR_capget             184
#define NR_capset             185
#define NR_sigaltstack        186
#define NR_sendfile           187
#define NR_getpmsg            188     /* some people actually want streams */
#define NR_putpmsg            189     /* some people actually want streams */
#define NR_vfork              190

typedef long ssize_t;
typedef unsigned long size_t;

/* Standard file descriptors */
#define STDIN_FILENO    0  /* Standard input */
#define STDOUT_FILENO   1  /* Standard output */
#define STDERR_FILENO   2  /* Standard error output */

static ssize_t write(int fd, const void *buf, size_t count)
{
	struct syscall_result res;
	res = syscall3(NR_write, fd, (unsigned long)buf, count);
	return res.val;
}

static void _exit(int status)
{
	struct syscall_result res;
	res = syscall1(NR_exit, status);
}

static const char *addr_of_char(unsigned char ch)
{
	static const char byte[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
		0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
		0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
		0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
		0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
		0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
		0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
		0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
		0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
		0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
		0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
		0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
		0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
		0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
		0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
		0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
		0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
		0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
		0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
		0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
		0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
		0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
		0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
		0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
		0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
	};
	return byte + ch;
}

static void console_tx_byte(unsigned char ch)
{
	write(STDOUT_FILENO, addr_of_char(ch), 1);
}

static void console_tx_nibble(unsigned nibble)
{
	unsigned char digit;
	digit = nibble + '0';
	if (digit > '9') {
		digit += 39;
	}
	console_tx_byte(digit);
}

static void console_tx_char(unsigned char byte)
{
	console_tx_byte(byte);
}

static void console_tx_hex8(unsigned char value)
{
	console_tx_nibble((value >> 4U) & 0x0fU);
	console_tx_nibble(value & 0x0fU);
}

static void console_tx_hex16(unsigned short value)
{
	console_tx_nibble((value >> 12U) & 0x0FU);
	console_tx_nibble((value >>  8U) & 0x0FU);
	console_tx_nibble((value >>  4U) & 0x0FU);
	console_tx_nibble(value & 0x0FU);
}

static void console_tx_hex32(unsigned short value)
{
	console_tx_nibble((value >> 28U) & 0x0FU);
	console_tx_nibble((value >> 24U) & 0x0FU);
	console_tx_nibble((value >> 20U) & 0x0FU);
	console_tx_nibble((value >> 16U) & 0x0FU);
	console_tx_nibble((value >> 12U) & 0x0FU);
	console_tx_nibble((value >>  8U) & 0x0FU);
	console_tx_nibble((value >>  4U) & 0x0FU);
	console_tx_nibble(value & 0x0FU);
}

static void console_tx_string(const char *str)
{
	unsigned char ch;
	while((ch = *str++) != '\0') {
		console_tx_byte(ch);
	}
}

static void print_emerg_char(unsigned char byte) { console_tx_char(byte); }
static void print_emerg_hex8(unsigned char value) { console_tx_hex8(value); }
static void print_emerg_hex16(unsigned short value){ console_tx_hex16(value); }
static void print_emerg_hex32(unsigned int value) { console_tx_hex32(value); }
static void print_emerg(const char *str) { console_tx_string(str); }

static void print_debug_char(unsigned char byte) { console_tx_char(byte); }
static void print_debug_hex8(unsigned char value) { console_tx_hex8(value); }
static void print_debug_hex16(unsigned short value){ console_tx_hex16(value); }
static void print_debug_hex32(unsigned int value) { console_tx_hex32(value); }
static void print_debug(const char *str) { console_tx_string(str); }


int log2(int value)
{
	/* __builtin_bsr is a exactly equivalent to the x86 machine
	 * instruction with the exception that it returns -1
	 * when the value presented to it is zero.
	 * Otherwise __builtin_bsr returns the zero based index of
	 * the highest bit set.
	 */
	return __builtin_bsr(value);
}


static void die(const char *str)
{
	print_emerg(str);
	do {
		asm(" ");
	} while(1);

}

static int smbus_read_byte(unsigned device, unsigned address)
{
	static const unsigned char dimm[] = {
0x80, 0x08, 0x07, 0x0d, 0x0a, 0x02, 0x48, 0x00, 0x04, 0x60, 0x70, 0x02, 0x82, 0x08, 0x08, 0x01,
0x0e, 0x04, 0x0c, 0x01, 0x02, 0x20, 0x00, 0x75, 0x70, 0x00, 0x00, 0x48, 0x30, 0x48, 0x2a, 0x40,
0x80, 0x80, 0x45, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

0x80, 0x08, 0x07, 0x0d, 0x0a, 0x02, 0x48, 0x00, 0x04, 0x60, 0x70, 0x02, 0x82, 0x08, 0x08, 0x01,
0x0e, 0x04, 0x0c, 0x01, 0x02, 0x20, 0x00, 0x75, 0x70, 0x00, 0x00, 0x48, 0x30, 0x48, 0x2a, 0x40,
0x80, 0x80, 0x45, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	};
	return dimm[(device << 8) + address];
}

#define SMBUS_MEM_DEVICE_START 0x00
#define SMBUS_MEM_DEVICE_END   0x01
#define SMBUS_MEM_DEVICE_INC   1

/* Function 2 */
#define DRAM_CONFIG_HIGH   0x94
#define  DCH_MEMCLK_SHIFT  20
#define  DCH_MEMCLK_MASK   7
#define  DCH_MEMCLK_100MHZ 0
#define  DCH_MEMCLK_133MHZ 2
#define  DCH_MEMCLK_166MHZ 5
#define  DCH_MEMCLK_200MHZ 7

/* Function 3 */
#define NORTHBRIDGE_CAP    0xE8
#define  NBCAP_128Bit         0x0001
#define  NBCAP_MP             0x0002
#define  NBCAP_BIG_MP         0x0004
#define  NBCAP_ECC            0x0004
#define  NBCAP_CHIPKILL_ECC   0x0010
#define  NBCAP_MEMCLK_SHIFT   5
#define  NBCAP_MEMCLK_MASK    3
#define  NBCAP_MEMCLK_100MHZ  3
#define  NBCAP_MEMCLK_133MHZ  2
#define  NBCAP_MEMCLK_166MHZ  1
#define  NBCAP_MEMCLK_200MHZ  0
#define  NBCAP_MEMCTRL        0x0100

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

static unsigned spd_to_dimm(unsigned device)
{
	return (device - SMBUS_MEM_DEVICE_START);
}

static void disable_dimm(unsigned index)
{
	print_debug("disabling dimm");
	print_debug_hex8(index);
	print_debug("\r\n");
#if 0
	pci_write_config32(PCI_DEV(0, 0x18, 2), DRAM_CSBASE + (((index << 1)+0)<<2), 0);
	pci_write_config32(PCI_DEV(0, 0x18, 2), DRAM_CSBASE + (((index << 1)+1)<<2), 0);
#endif
}


struct mem_param {
	uint8_t cycle_time;
	uint32_t dch_memclk;
};

static const struct mem_param *get_mem_param(unsigned min_cycle_time)
{
	static const struct mem_param speed[] = {
		{
			.cycle_time = 0xa0,
			.dch_memclk = DCH_MEMCLK_100MHZ << DCH_MEMCLK_SHIFT,
		},
		{
			.cycle_time = 0x75,
			.dch_memclk = DCH_MEMCLK_133MHZ << DCH_MEMCLK_SHIFT,
		},
		{
			.cycle_time = 0x60,
			.dch_memclk = DCH_MEMCLK_166MHZ << DCH_MEMCLK_SHIFT,
		},
		{
			.cycle_time = 0x50,
			.dch_memclk = DCH_MEMCLK_200MHZ << DCH_MEMCLK_SHIFT,
		},
		{
			.cycle_time = 0x00,
		},
	};
	const struct mem_param *param;
	for(param = &speed[0]; param->cycle_time ; param++) {
		if (min_cycle_time > (param+1)->cycle_time) {
			break;
		}
	}
	if (!param->cycle_time) {
		die("min_cycle_time to low");
	}
	return param;
}

#if 1
static void debug(int c)
{
	print_debug_char(c);
	print_debug_char('\r');
	print_debug_char('\n');
}
#endif
static const struct mem_param *spd_set_memclk(void)
{
	/* Compute the minimum cycle time for these dimms */
	const struct mem_param *param;
	unsigned min_cycle_time, min_latency;
	unsigned device;
	uint32_t value;

	static const int latency_indicies[] = { 26, 23, 9 };
	static const unsigned char min_cycle_times[] = {
		[NBCAP_MEMCLK_200MHZ] = 0x50, /* 5ns */
		[NBCAP_MEMCLK_166MHZ] = 0x60, /* 6ns */
		[NBCAP_MEMCLK_133MHZ] = 0x75, /* 7.5ns */
		[NBCAP_MEMCLK_100MHZ] = 0xa0, /* 10ns */
	};


#if 0
	value = pci_read_config32(PCI_DEV(0, 0x18, 3), NORTHBRIDGE_CAP);
#else
	value = 0x50;
#endif
	min_cycle_time = min_cycle_times[(value >> NBCAP_MEMCLK_SHIFT) & NBCAP_MEMCLK_MASK];
	min_latency = 2;

#if 1
	print_debug("min_cycle_time: ");
	print_debug_hex8(min_cycle_time);
	print_debug(" min_latency: ");
	print_debug_hex8(min_latency);
	print_debug("\r\n");
#endif

	/* Compute the least latency with the fastest clock supported
	 * by both the memory controller and the dimms.
	 */
	for(device = SMBUS_MEM_DEVICE_START;
		device <= SMBUS_MEM_DEVICE_END;
		device += SMBUS_MEM_DEVICE_INC)
	{
		int new_cycle_time, new_latency;
		int index;
		int latencies;
		int latency;

		debug('A');
		/* First find the supported CAS latencies
		 * Byte 18 for DDR SDRAM is interpreted:
		 * bit 0 == CAS Latency = 1.0
		 * bit 1 == CAS Latency = 1.5
		 * bit 2 == CAS Latency = 2.0
		 * bit 3 == CAS Latency = 2.5
		 * bit 4 == CAS Latency = 3.0
		 * bit 5 == CAS Latency = 3.5
		 * bit 6 == TBD
		 * bit 7 == TBD
		 */
		new_cycle_time = 0xa0;
		new_latency = 5;

		latencies = smbus_read_byte(device, 18);
		if (latencies <= 0) continue;

		debug('B');
		/* Compute the lowest cas latency supported */
		latency = log2(latencies) -2;

		/* Loop through and find a fast clock with a low latency */
		for(index = 0; index < 3; index++, latency++) {
			int value;
			debug('C');
			if ((latency < 2) || (latency > 4) ||
				(!(latencies & (1 << latency)))) {
				continue;
			}
			debug('D');
			value = smbus_read_byte(device, latency_indicies[index]);
			if (value < 0) continue;

			debug('E');
			/* Only increase the latency if we decreas the clock */
			if ((value >= min_cycle_time) && (value < new_cycle_time)) {
				new_cycle_time = value;
				new_latency = latency;
#if 1
				print_debug("device: ");
				print_debug_hex8(device);
				print_debug(" new_cycle_time: ");
				print_debug_hex8(new_cycle_time);
				print_debug(" new_latency: ");
				print_debug_hex8(new_latency);
				print_debug("\r\n");
#endif
			}
			debug('G');
		}
		debug('H');
#if 1
		print_debug("device: ");
		print_debug_hex8(device);
		print_debug(" new_cycle_time: ");
		print_debug_hex8(new_cycle_time);
		print_debug(" new_latency: ");
		print_debug_hex8(new_latency);
		print_debug("\r\n");
#endif
		if (new_latency > 4){
			continue;
		}
		debug('I');
		/* Does min_latency need to be increased? */
		if (new_cycle_time > min_cycle_time) {
			min_cycle_time = new_cycle_time;
		}
		/* Does min_cycle_time need to be increased? */
		if (new_latency > min_latency) {
			min_latency = new_latency;
		}
#if 1
		print_debug("device: ");
		print_debug_hex8(device);
		print_debug(" min_cycle_time: ");
		print_debug_hex8(min_cycle_time);
		print_debug(" min_latency: ");
		print_debug_hex8(min_latency);
		print_debug("\r\n");
#endif
	}
	/* Make a second pass through the dimms and disable
	 * any that cannot support the selected memclk and cas latency.
	 */
	for(device = SMBUS_MEM_DEVICE_START;
		device <= SMBUS_MEM_DEVICE_END;
		device += SMBUS_MEM_DEVICE_INC)
	{
		int latencies;
		int latency;
		int index;
		int value;
		int dimm;
		latencies = smbus_read_byte(device, 18);
		if (latencies <= 0) {
			goto dimm_err;
		}

		/* Compute the lowest cas latency supported */
		latency = log2(latencies) -2;

		/* Walk through searching for the selected latency */
		for(index = 0; index < 3; index++, latency++) {
			if (!(latencies & (1 << latency))) {
				continue;
			}
			if (latency == min_latency)
				break;
		}
		/* If I can't find the latency or my index is bad error */
		if ((latency != min_latency) || (index >= 3)) {
			goto dimm_err;
		}

		/* Read the min_cycle_time for this latency */
		value = smbus_read_byte(device, latency_indicies[index]);

		/* All is good if the selected clock speed
		 * is what I need or slower.
		 */
		if (value <= min_cycle_time) {
			continue;
		}
		/* Otherwise I have an error, disable the dimm */
	dimm_err:
		disable_dimm(spd_to_dimm(device));
	}
#if 1
	print_debug("min_cycle_time: ");
	print_debug_hex8(min_cycle_time);
	print_debug(" min_latency: ");
	print_debug_hex8(min_latency);
	print_debug("\r\n");
#endif
	/* Now that I know the minimum cycle time lookup the memory parameters */
	param = get_mem_param(min_cycle_time);

#if 0
	/* Update DRAM Config High with our selected memory speed */
	value = pci_read_config32(PCI_DEV(0, 0x18, 2), DRAM_CONFIG_HIGH);
	value &= ~(DCH_MEMCLK_MASK << DCH_MEMCLK_SHIFT);
	value |= param->dch_memclk;
	pci_write_config32(PCI_DEV(0, 0x18, 2), DRAM_CONFIG_HIGH, value);

	static const unsigned latencies[] = { 1, 5, 2 };
	/* Update DRAM Timing Low wiht our selected cas latency */
	value = pci_read_config32(PCI_DEV(0, 0x18, 2), DRAM_CONFIG_LOW);
	value &= ~7;
	value |= latencies[min_latency - 2];
	pci_write_config32(PCI_DEV(0, 0x18, 2), DRAM_CONFIG_LOW, value);
#endif

	return param;
}

static void main(void)
{
	const struct mem_param *param;
	param = spd_set_memclk();
	_exit(0);
}
