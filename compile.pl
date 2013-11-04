#!/usr/bin/perl

$allocs_i = 0;
$allocs = {};
$funcs = [];
$current;
$key;
while(<STDIN>)
{
	~s/^\s+//;
	~s/\s+$//;
	if($_ eq "" || m/^#/)
	{
		next;
	}
	if(m/^alloc(\[(.*)\])?:$/)
	{
		$allocs_i++;
		$key = 'alloc';
		if($2)
		{
			if(!exists($allocs{$2}))
			{
				$allocs{$2} = ();
			}
			$current = $2;
		}
		else
		{
			$allocs{$allocs_i} = ();
			$current = $allocs_i;
		}
	}
	elsif(m/^dealloc:$/)
	{
		$key = 'dealloc';
	}
	elsif(m/^use:$/)
	{
		$key = 'use';
	}
	elsif(m/^functions?:$/)
	{
		$key = 'functions';
	}
	else
	{
		# print "$_\n";
		if($key eq 'functions')
		{
			push($funcs, $_);
		}
		else
		{
			if(!exists($allocs{$current}{$key}))
			{
				$allocs{$current}{$key} = [];
				# print;
				# $allocs{"$current"}{"$key"} = 123;
			}
			push($allocs{$current}{$key}, $_);
		}
	}
}

print "<?xml version=\"1.0\"?>\n<def>\n";

while(($key, $value) = each(%allocs))
{
	print "\t<memory>\n";
	while(($k, $v) = each($value))
	{
		# print $k;
		while(($kk, $vv) = each($v))
		{
			print "\t\t<$k>$vv</$k>\n";
		}
	}
	print "\t</memory>\n";
}

while(($k, $v) = each($funcs))
{
	my @opts = split(/\s+/, $v);
	print "\t<function name=\"@opts[0]\">\n";
	for($i = 1; $i < @opts; $i++)
	{
		if(@opts[$i] eq "leak-ignore")
		{
			print "\t\t<leak-ignore/>\n";
		}
		elsif(@opts[$i] eq "return")
		{
			print "\t\t<noreturn>false</noreturn>\n";
		}
		elsif(@opts[$i] eq "noreturn")
		{
			print "\t\t<noreturn>true</noreturn>\n";
		}
	}
	print "\t</function>\n";
}

print "</def>\n";
