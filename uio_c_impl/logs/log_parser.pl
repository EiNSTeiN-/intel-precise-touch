#!/usr/bin/perl

use strict; 


sub conv_p
{
	my ($p) = @_; 
	my $ret="UNKNOWN              ";
	$ret="VENDOR_ID            " if ($p eq "0x0");
	$ret="DEVICE_ID            " if ($p eq "0x2");
	$ret="PCI_COMMAND          " if ($p eq "0x4");
	$ret="PCI_STATUS           " if ($p eq "0x6");
	$ret="PCI_CLASS_REVISION   " if ($p eq "0x8");
	$ret="PCI_CLASS_PROG       " if ($p eq "0x9");
	$ret="PCI_CLASS_DEVICE     " if ($p eq "0xa");
	$ret="PCI_CLASS_DEVICE?    " if ($p eq "0xb");
	$ret="PCI_CACHE_LINE_SIZE  " if ($p eq "0xc");
	$ret="PCI_LATENCY_TIMER    " if ($p eq "0xd");
	$ret="PCI_HEADER_TYPE      " if ($p eq "0xe");
	$ret="PCI_BASE_ADDRESS_0   " if ($p eq "0x10");
	$ret="PCI_BASE_ADDRESS_1   " if ($p eq "0x14");
	$ret="PCI_BASE_ADDRESS_2   " if ($p eq "0x18");
	$ret="PCI_BASE_ADDRESS_3   " if ($p eq "0x1c");
	$ret="PCI_BASE_ADDRESS_4   " if ($p eq "0x20");
	$ret="PCI_BASE_ADDRESS_5   " if ($p eq "0x24");
	$ret="PCI_CARDBUS_CIS      " if ($p eq "0x28");
	$ret="PCI_CARDBUS_CIS      " if ($p eq "0x28");
	$ret="PCI_SUBSYS_VENDOR_ID " if ($p eq "0x2c");
	$ret="PCI_SUBSYS_ID        " if ($p eq "0x2e");
	$ret="PCI_ROM_ADDRESS      " if ($p eq "0x30");
	$ret="PCI_CAPABILITY_LIST  " if ($p eq "0x34");
	$ret="PCI_INTERRUPT_LINE   " if ($p eq "0x3c");
	$ret="PCI_INTERRUPT_PIN    " if ($p eq "0x3d");
	return $ret; 
}


while (<>){
	my $l=$_; 

    if ($l =~ /read.*(0000:00:[0-9\.]+), @(.*), len=(.*)\) (.*)/  ) {
		my ($dev, $p, $len, $res) = ($1,$2,$3,$4,$5);
	print "$dev, READ  ".conv_p($p)."($p), len:$len, res:$res\n";
    }
    elsif ($l =~ /(write).*(0000:00:[0-9\.]+), @(.*), (.*), len=(.*)\)/  ) {
		my ($op, $dev, $p, $val, $len) = ($1,$2,$3,$4,$5);
	print "$dev, WRITE ".conv_p($p)."($p), val:$val, len:$len\n";
    }
    else { print "??? $l"; }


}

