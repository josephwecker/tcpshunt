
#include <net/sock.h>
#include <net/inet_sock.h>
#include <linux/types.h>


// ioctl module...

	struct inet_sock *inet = inet_sk(sk);
	__be32 old_saddr = inet->inet_saddr;
	__be32 new_saddr = inet->inet_saddr;



	inet->inet_saddr = inet->inet_rcv_saddr = new_saddr;

	__sk_prot_rehash(sk);



