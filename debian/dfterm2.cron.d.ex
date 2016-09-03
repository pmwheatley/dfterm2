#
# Regular cron jobs for the dfterm2 package
#
0 4	* * *	root	[ -x /usr/bin/dfterm2_maintenance ] && /usr/bin/dfterm2_maintenance
