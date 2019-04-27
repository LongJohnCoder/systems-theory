#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */
#include <linux/netfilter.h> 
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/compiler.h>
#include <linux/mutex.h>
#include <net/tcp.h>

/* make IP4-addresses readable */

#define NIPQUAD(addr) \
        ((unsigned char *)&addr)[0], \
        ((unsigned char *)&addr)[1], \
        ((unsigned char *)&addr)[2], \
        ((unsigned char *)&addr)[3]


MODULE_AUTHOR ("Eike Ritter <E.Ritter@cs.bham.ac.uk>");
MODULE_DESCRIPTION ("Extensions to the firewall") ;
MODULE_LICENSE("GPL");




struct nf_hook_ops *reg;

unsigned int FirewallExtensionHook (unsigned int hooknum,
				    struct sk_buff *skb,
				    const struct net_device *in,
				    const struct net_device *out,

				    int (*okfn)(struct sk_buff *)) {

    	const struct iphdr *iph = ip_hdr(skb);
	struct udphdr _hdr, *hp = NULL;
	//	const struct tcphdr *th;
	//	struct tcphdr _tcph;
	u8 uninitialized_var(protocol);


	__be32 uninitialized_var(daddr), uninitialized_var(saddr);
	__be16 uninitialized_var(dport), uninitialized_var(sport);

	/* check whether it is a UDP-protocol */
	if (iph->protocol == IPPROTO_UDP) {
	    hp = skb_header_pointer(skb, ip_hdrlen(skb),
				    sizeof(_hdr), &_hdr);
	    if (hp == NULL) 
		return NF_ACCEPT;
	    protocol = iph->protocol;
	    saddr = iph->saddr;
	    sport = hp->source;
	    daddr = iph->daddr;
	    dport = hp->dest;
	    


	    printk (KERN_INFO "firewall: UDP-packet \n");
		
	    printk (KERN_INFO "firewall: Source address %u.%u.%u.%u\n", NIPQUAD(saddr));
	    printk (KERN_INFO "firewall: destination port = %d\n", htons(dport)); 
	    if (htons (dport) == 53) {
		//	    printk (KERN_INFO "firewall: Starting connection \n");
		
		//	    printk (KERN_INFO "firewall: Source address %u.%u.%u.%u\n", NIPQUAD(saddr));
		printk (KERN_INFO "DNS-packet found, rejecting\n");
	    return NF_DROP;

	}
	}
	//    }
    return NF_ACCEPT;	
}

EXPORT_SYMBOL (FirewallExtensionHook);


int init_module(void)
{

  int errno;

  /* allocate space for hook */
  reg = kmalloc (sizeof (struct nf_hook_ops), GFP_KERNEL);
  if (!reg) {
    return -ENOMEM;
  }

  /* fill it with the right data */
  reg->hook = FirewallExtensionHook; /* the procedure to be called */
  reg->pf = PF_INET;
  reg->owner = THIS_MODULE;
  reg->hooknum = NF_INET_LOCAL_OUT; /* only for incoming connections */

  errno = nf_register_hook (reg); /* register the hook */
  if (errno) {
    printk (KERN_INFO "Firewall extension could not be registered!\n");
    kfree (reg);
  } 
  else {
    printk(KERN_INFO "Firewall extensions module loaded\n");
  }

  // A non 0 return means init_module failed; module can't be loaded.
  return errno;
}


void cleanup_module(void)
{

	nf_unregister_hook (reg); /* restore everything to normal */
	kfree (reg);
	printk(KERN_INFO "Firewall extensions module unloaded\n");
}  
