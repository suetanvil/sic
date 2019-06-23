#!/usr/bin/env perl

# Script to produce a Markdown reference document for the internal
# functions from sic_func.inc

use strict;
use warnings;


my @Funcs = ();
my %Aliases = ();

sub add_func {
  my ($cname, $min_args, $is_vararg, $is_macro, $comment) = @_;

  my $name = $cname;
  $name =~ s/_p$/?/;
  $name =~ s/_op$//;
  $name =~ s/_/-/g;

  push @Funcs, {name        => $name,
                cname       => $cname,
                min_args    => $min_args,
                is_vararg   => $is_vararg,
                is_macro    => $is_macro,
                comment     => $comment};
}

sub format_entry {
  my ($entry) = @_;

  print "## \`$entry->{name}\`";

  my $paren = 0;
  if ($Aliases{$entry->{name}}) {
    my @al = map{"\`$_\`"} @{ $Aliases{ $entry->{name} } };
    print " (also ", join(", ", @al);
    $paren = 1;
  }

  if ($entry->{name} ne $entry->{cname}) {
    print $paren ? ", " : " (";
    $paren = 1;
    print "`$entry->{cname}\` in C++";
  }

  print ")" if $paren;
  print "\n\n";

  my @desc = @{ $entry->{comment} };

  if ($desc[0] !~ /^ *\(/) {
    my $signature = "";
    $signature = "`($entry->{name}";
    $signature .= " arg$_" for (1..$entry->{min_args});
    $signature .= " ..." if $entry->{is_vararg};
    $signature .= ")`";
    unshift @desc, "";
    unshift @desc, $signature;
  } else {
    my $signature = shift @desc;
    $signature =~ s/^\w*//;
    $signature =~ s/\w*$//;
    unshift @desc, "\`$signature\`";
  }

  print shift @desc, "\n";

  print "\n**Macro**\n" if $entry->{is_macro};

  my $desc = join("\n", @desc);
  $desc =~ s/\n\n+/\n\n/gmx;

  print "$desc\n\n";
}


# Read the source file
{
  my @comment = ();

  LINE: while(<>) {
    next if /^\s*$/;

    m{^///} and do {
      s{^///\s*}{};
      chomp;
      push @comment, $_;
      next LINE;
    };

    m{^ALIAS} and do {
      m{^ALIAS \s* \( (\w+) \s* \, \s* \" ([^\"]+) \" \)}x
        or die "Malformed 'ALIAS': '$_'\n";
      $Aliases{$1} = [] unless $Aliases{$1};
      push @{ $Aliases{$1} }, $2;

      next LINE;
    };

    m{^BUILTIN_FULL} and do {
      s/\s+//g;
      m{^BUILTIN_FULL \( (\w+) \, (\d+) \, (true|false) \, (true|false) \)}x
        or die "Malformed BUILTIN_FULL: '$_'\n";
      add_func($1, $2, $3 eq 'true', $4 eq 'true', [@comment]);
      @comment = ();
    };

    m{^BUILTIN\s*\(} and do {
      s/\s+//g;
      m{^BUILTIN \( (\w+) \, (\d+) \)}x
        or die "Malformed BUILTIN: '$_'\n";
      add_func($1, $2, 0, 0, [@comment]);
      @comment = ();
    };
  }

  print <<"EOF";
# Sic functions

These are the built-in functions and macros defined by the `sic`
programming language.

This document was generated on @{[scalar(localtime())]}.

EOF
    ;


  @Funcs = sort{ $a->{name} cmp $b->{name} } @Funcs;
  for (@Funcs) {
    format_entry($_);
  }
}
