#!/usr/bin/perl

use POSIX ();

sub aexit {
	open F, "ps -eo ppid,pid |";
	%h = ();
	while (<F>) {
		chomp;
		($ppid, $pid) = /(\S+)/g;
		$h{$pid} = $ppid;
	}
	%a = ($$, 1);
	while (1) {
		$n = 0;
		for $pid (keys %h) {
			$ppid = $h{$pid};
			if ($a{$ppid} && !$a{$pid}) {
				$a{$pid} = 1;
				push @b, $pid;
				$n++;
			}
		}
		last if !$n;
	}
	kill 15, @b;
	exit;
}

sub exit_kill_child {
	$SIG{TERM} = sub { aexit; };
}

1;

