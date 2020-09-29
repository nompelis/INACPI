# IN ACPI (for laptop batteries)

This is my little daemon for monitoring a Linux (kernel 2.6.x) laptop's
battery. It issues a shutdown warning at 300 mAh capacity and forces a
shutdown at 100 mAh capacity.

It tries to automatically find the battery. If it fails, it will issue a
shutdown immediatelly. In fact, the safe thing to do is to issue a shutdown
on all errors (and it does) because should the battery capacity run out and
the sytem shut power ungracefully, unmounting of filesystemso will not happen,
and this may result in possible filesystem corruption, or worse hard-disk
damage. (Yeah, cut the power when the head is not "parcked" and watch the
carnage.)

IN 2020/09/29

