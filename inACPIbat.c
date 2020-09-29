/************************************************************************
  A light-weight daemon to monitor a Linux laptop's battery via ACIDd.

  Copyright 2020 Ioannis Nompelis
  Ioannis Nompelis  <nompelis@nobelware.com>           Created: 20200920
  Ioannis Nompelis  <nompelis@nobelware.com>     Last modified: 20200929
 ************************************************************************/

#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <syslog.h>
#include <unistd.h>


#define NUM_TOTAL   60

struct inACPIbat_data_s {
   int bat_num;
   int state;
   int num_entries;
   int cur_entry;
   unsigned int interval;
   int mAh_table[ NUM_TOTAL ];
};

enum inACPIbat_state {
   BATTERY_CHARGED,
   BATTERY_CHARGING,
   BATTERY_DISCHARGING,
};

enum inACPIbat_capacity {
   CAPACITY_WARNING = 200,
   CAPACITY_HALT = 100,
};


void inACPIbat_ClearStruct( struct inACPIbat_data_s *p )
{
   int n;

   p->bat_num = -1;
   p->state = -1;
   p->num_entries = 0;
   p->cur_entry = -1;
   p->interval = 60;

   for(n=0;n<NUM_TOTAL;++n) {
      p->mAh_table[n] = -1;
   }
}


void inACPIbat_Report_HaltWarning( struct inACPIbat_data_s *p )
{
   char msg[256];

   sprintf( msg, "[inACPIbat]  Battery: %d capacity low \n", p->bat_num );
   syslog( 0, "%s", msg );    // do it this way according to the man page
   sprintf( msg, "[inACPIbat]  Will halt the system now!\n");
   syslog( 0, "%s", msg );    // do it this way according to the man page

   sprintf( msg, "/sbin/shutdown -t 10 -h now \"Low battery. Shutodwn imminent!\" " );
// system( msg );
}


void inACPIbat_Report_BatteryWarning( struct inACPIbat_data_s *p )
{
   char msg[256];

   sprintf( msg, "[inACPIbat]  Battery: %d capacity low \n", p->bat_num );
   syslog( 0, "%s", msg );    // do it this way according to the man page
   sprintf( msg, "[inACPIbat]  Remaining capacity: %d mAh \n",
            p->mAh_table[ p->cur_entry ] );
   syslog( 0, "%s", msg );    // do it this way according to the man page

   sprintf( msg, "/sbin/shutdown -t 10 -k now \"Battery low; shuting down soon...\" " );
// system( msg );
}


void inACPIbat_Report_NotFound( )
{
   char msg[256];

   sprintf( msg, "[inACPIbat]  Battery not found\n");
   syslog( 0, "%s", msg );    // do it this way according to the man page
}


void inACPIbat_Report_Found( struct inACPIbat_data_s *p )
{
   char msg[256];

   sprintf( msg, "[inACPIbat]  Found battery: %d\n", p->bat_num );
   syslog( 0, "%s", msg );    // do it this way according to the man page

   if( p->state == BATTERY_DISCHARGING ) {
      sprintf( msg, "[inACPIbat]  Battery %d is discharging \n", p->bat_num );
   } else if( p->state == BATTERY_CHARGING ) {
      sprintf( msg, "[inACPIbat]  Battery %d is charging \n", p->bat_num );
   } else if( p->state == BATTERY_CHARGED ) {
      sprintf( msg, "[inACPIbat]  Battery %d is charged \n", p->bat_num );
   }
   syslog( 0, "%s", msg );    // do it this way according to the man page
}


void inACPIbat_Report_CouldNotQuery( struct inACPIbat_data_s *p )
{
   char msg[256];

   sprintf( msg, "[inACPIbat]  Cannot query battery: %d\n", p->bat_num );
   syslog( 0, "%s", msg );    // do it this way according to the man page
   inACPIbat_Report_HaltWarning( p );
}


void inACPIbat_Report_NotPresent( struct inACPIbat_data_s *p )
{
   char msg[256];

   sprintf( msg, "[inACPIbat]  Battery: %d is NOT present!\n", p->bat_num );
   syslog( 0, "%s", msg );    // do it this way according to the man page
   inACPIbat_Report_HaltWarning( p );
}


void inACPIbat_Report_ErrorState( struct inACPIbat_data_s *p )
{
   char msg[256];

   sprintf( msg, "[inACPIbat]  Battery: %d state query failed!\n", p->bat_num );
   syslog( 0, "%s", msg );    // do it this way according to the man page
   inACPIbat_Report_HaltWarning( p );
}




int inACPIbat_FindBattery( struct inACPIbat_data_s *p )
{
   int n,iok;
   char proc_file[256];
   FILE *fp;
   char data[256];

   iok = -1;
   n = 0;
   while( n < 100 && iok == -1 ) {
      sprintf( proc_file, "/proc/acpi/battery/BAT%d/state", n );
      fp = fopen( proc_file, "r" );

      if( fp != NULL ) {
         char *s1;

         fgets( data, 256, fp );
         s1 = strtok( data, ":");
     //  printf("s1: \"%s\"\n",s1);//HACK
         s1 = strtok( NULL, " ");
     //  printf("s1: \"%s\"\n",s1);//HACK
         if( strncmp( s1, "y", 0 ) == 0 ) {
         // printf("Battery present: %d \n",n);//HACK
            p->bat_num = n;
            iok = 0;
         } else {
            // this battery is "not present"
         }

         fclose( fp );
      }
      ++n;
   }

   if( iok != 0 ) return 1;

   return 0;
}


int inACPIbat_GetState( struct inACPIbat_data_s *p )
{
   char data[256];
   FILE *fp;
   int iok = 0;

   sprintf( data, "/proc/acpi/battery/BAT%d/state", p->bat_num );
   fp = fopen( data, "r" );
   if( fp == NULL ) {
      inACPIbat_Report_CouldNotQuery( p );
      return 1;
   }

   while( !feof( fp ) ) {
      char *s1;

      fgets( data, 256, fp );
      if( strncmp( data, "present:", 8 ) == 0 ) {
         s1 = strtok( data, ":" );
         s1 = strtok( NULL, " " );
         if( strncmp( s1, "yes", 3 ) == 0 ) {
         // printf("Battery %d is present \n", p->bat_num);//HACK
            iok++;
         } else {
            inACPIbat_Report_NotPresent( p );
         }
      }

      if( strncmp( data, "charging state:", 15 ) == 0 ) {
         s1 = strtok( data, ":" );
         s1 = strtok( NULL, " " );
         if( strncmp( s1, "discharging", 11 ) == 0 ) {
         // printf("Battery %d is discharging \n", p->bat_num);//HACK
            p->state = BATTERY_DISCHARGING;
         }
         if( strncmp( s1, "charging", 8 ) == 0 ) {
         // printf("Battery %d is charging \n", p->bat_num);//HACK
            p->state = BATTERY_CHARGING;
         }
         if( strncmp( s1, "charged", 7 ) == 0 ) {
         // printf("Battery %d is charged \n", p->bat_num);//HACK
            p->state = BATTERY_CHARGED;
         }
         iok++;
      }

      if( strncmp( data, "remaining capacity:", 19 ) == 0 ) {
         int irem;

         s1 = strtok( data, ":" );
         s1 = strtok( NULL, ":" );
         sscanf( s1, "%d", &irem );
      // printf("Battery %d rem. capacity %d mAh \n", p->bat_num, irem );//HACK
         iok++;

         // store the present capacity
         if( p->num_entries < NUM_TOTAL ) ++( p->num_entries );
         ++( p->cur_entry );
         if( p->cur_entry == NUM_TOTAL ) {
            p->cur_entry = 0;
         }
         p->mAh_table[ p->cur_entry ] = irem;
      }

   }

   fclose( fp );

   if( iok != 3 ) return 2;

   return 0;
}


int inACPIbat_Cycle( struct inACPIbat_data_s *p )
{
   int iret;


   while( 1 ) {
#ifdef _DEBUG_
      printf("About to query battery %d state \n", p->bat_num );
#endif
      iret = inACPIbat_GetState( p );
      if( iret != 0 ) {
         inACPIbat_Report_ErrorState( p );
         return 2;
      }

#ifdef _DEBUG_
      printf(" - Battery %d state %d, capacity: %d \n",
             p->bat_num, p->state, p->mAh_table[ p->cur_entry ] );
      printf("   Entries: %d, current entry: %d \n",
             p->num_entries, p->cur_entry );
#endif

      if( p->mAh_table[ p->cur_entry ] <= CAPACITY_HALT ) {
         inACPIbat_Report_HaltWarning( p );
         return 0;
      } else if( p->mAh_table[ p->cur_entry ] <= CAPACITY_WARNING ) {
         inACPIbat_Report_BatteryWarning( p );
      } else {
#ifdef _DEBUG_
         printf("Capacity still good enough \n");
#endif
      }

      sleep( p->interval );
   }

   return 0;
}



// Driver //

int main( int argc, char* argv[] )
{
   static struct inACPIbat_data_s s;
   int iret;

#ifndef _DEBUG_
   daemon( 0, 0 );
#endif

   inACPIbat_ClearStruct( &s );

   iret = inACPIbat_FindBattery( &s );
   if( iret != 0 ) {
      inACPIbat_Report_NotFound();
      return 1;
   } else {
      inACPIbat_Report_Found( &s );
   }

   iret = inACPIbat_Cycle( &s );
   if( iret != 0 ) return 3;

   return EXIT_SUCCESS;
}
