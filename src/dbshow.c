#include "common.h"
#include "dbsql.h"
#include "misc.h"
#include "dbshow.h"

void showdb(const char *interface, int qmode)
{
	interfaceinfo info;

	if (!db_getinterfacecountbyname(interface)) {
		return;
	}

	if (!db_getinterfaceinfo(interface, &info)) {
		return;
	}

	if (info.rxtotal == 0 && info.txtotal == 0 && qmode != 4) {
		printf(" %s: Not enough data available yet.\n", interface);
		return;
	}

	switch(qmode) {
		case 0:
			showsummary(&info);
			break;
/*		case 1:
			showdays();
			break;
		case 2:
			showmonths();
			break;
		case 3:
			showtop();
			break;
		case 4:
			exportdb();
			break;
		case 5:
			showshort();
			break;
		case 6:
			showweeks();
			break;
		case 7:
			showhours();
			break;
		case 9:
			showoneline();
			break;*/
		default:
			printf("Error: Not such query mode: %d\n", qmode);
			break;
	}
}

void showsummary(interfaceinfo *interface)
{
	struct tm *d;
	char datebuff[DATEBUFFLEN];
	char todaystr[DATEBUFFLEN], yesterdaystr[DATEBUFFLEN];
	uint64_t e_rx, e_tx;
	time_t current, yesterday;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	current=time(NULL);
	yesterday=current-86400;

	e_rx=e_tx=0;

	if (interface->updated) {
		printf("Database updated: %s\n",(char*)asctime(localtime(&interface->updated)));
	} else {
		printf("\n");
	}

	indent(3);
	if (strcmp(interface->name, interface->alias) == 0 || strlen(interface->alias) == 0) {
		printf("%s", interface->name);
	} else {
		printf("%s (%s)", interface->alias, interface->name);
	}
	if (interface->active == 0) {
		printf(" [disabled]");
	}

	/* get formatted date for creation date */
	d=localtime(&interface->created);
	strftime(datebuff, DATEBUFFLEN, cfg.tformat, d);
	printf(" since %s\n\n", datebuff);

	indent(10);
	printf("rx:  %s", getvalue(interface->rxtotal, 1, 1));
	indent(3);
	printf("   tx:  %s", getvalue(interface->txtotal, 1, 1));
	indent(3);
	printf("   total:  %s\n\n", getvalue(interface->rxtotal+interface->txtotal, 1, 1));

	indent(3);
	printf("monthly\n");
	indent(5);

	if (cfg.ostyle >= 2) {
		printf("                rx      |     tx      |    total    |   avg. rate\n");
		indent(5);
		printf("------------------------+-------------+-------------+---------------\n");
	} else {
		printf("                rx      |     tx      |    total\n");
		indent(5);
		printf("------------------------+-------------+------------\n");
	}


	if (!db_getdata(&datalist, &datainfo, interface->name, "month", 2, 0)) {
		/* TODO: match with other output style */
		printf("Error: failed to fetch monthly data\n");
		return;
	}

	datalist_i = datalist;

	while (datalist_i != NULL) {
		indent(5);
		d = localtime(&datalist_i->timestamp);
		if (strftime(datebuff, DATEBUFFLEN, cfg.mformat, d)<=8) {
			printf("%*s   %s", getpadding(9, datebuff), datebuff, getvalue(datalist_i->rx, 11, 1));
		} else {
			printf("%-*s %s", getpadding(11, datebuff), datebuff, getvalue(datalist_i->rx, 11, 1));
		}
		printf(" | %s", getvalue(datalist_i->tx, 11, 1));
		printf(" | %s", getvalue(datalist_i->rx+datalist_i->tx, 11, 1));
		if (cfg.ostyle >= 2) {
			if (datalist_i->next == NULL) {
				printf(" | %s", gettrafficrate(datalist_i->rx+datalist_i->tx, mosecs(datalist_i->timestamp, interface->updated), 14));
			} else {
				printf(" | %s", gettrafficrate(datalist_i->rx+datalist_i->tx, dmonth(d->tm_mon)*86400, 14));
			}
		}
		printf("\n");
		if (datalist_i->next == NULL) {
			break;
		}
		datalist_i = datalist_i->next;
	}

	indent(5);
	if (cfg.ostyle >= 2) {
		printf("------------------------+-------------+-------------+---------------\n");
	} else {
		printf("------------------------+-------------+------------\n");
	}

	/* use database update time for estimates */
	if ( datalist_i->rx == 0 || datalist_i->tx == 0 || (interface->updated-datalist_i->timestamp) == 0 ) {
		e_rx = e_tx = 0;
	} else {
		e_rx = (datalist_i->rx/(float)(mosecs(datalist_i->timestamp, interface->updated)))*(dmonth(d->tm_mon)*86400);
		e_tx = (datalist_i->tx/(float)(mosecs(datalist_i->timestamp, interface->updated)))*(dmonth(d->tm_mon)*86400);
	}
	indent(5);
	printf("estimated   %s", getvalue(e_rx, 11, 2));
	printf(" | %s", getvalue(e_tx, 11, 2));
	printf(" | %s", getvalue(e_rx+e_tx, 11, 2));
	if (cfg.ostyle >= 2) {
		printf(" |\n\n");
	} else {
		printf("\n\n");
	}

	dbdatalistfree(&datalist);

	indent(3);
	printf("daily\n");
	indent(5);

	if (cfg.ostyle >= 2) {
		printf("                rx      |     tx      |    total    |   avg. rate\n");
		indent(5);
		printf("------------------------+-------------+-------------+---------------\n");
	} else {
		printf("                rx      |     tx      |    total\n");
		indent(5);
		printf("------------------------+-------------+------------\n");
	}

	/* get formatted date for today and yesterday */
	d = localtime(&current);
	strftime(todaystr, DATEBUFFLEN, cfg.dformat, d);
	d = localtime(&yesterday);
	strftime(yesterdaystr, DATEBUFFLEN, cfg.dformat, d);

	if (!db_getdata(&datalist, &datainfo, interface->name, "day", 2, 0)) {
		/* TODO: match with other output style */
		printf("Error: failed to fetch daily data\n");
		return;
	}

	datalist_i = datalist;

	while (datalist_i != NULL) {
		indent(5);
		d = localtime(&datalist_i->timestamp);
		strftime(datebuff, DATEBUFFLEN, cfg.dformat, d);
		if (strcmp(datebuff, todaystr) == 0) {
			snprintf(datebuff, DATEBUFFLEN, "    today");
		}
		if (strcmp(datebuff, yesterdaystr) == 0) {
			snprintf(datebuff, DATEBUFFLEN, "yesterday");
		}
		if (strlen(datebuff) <= 8) {
			printf("%*s   %s", getpadding(9, datebuff), datebuff, getvalue(datalist_i->rx, 11, 1));
		} else {
			printf("%-*s %s", getpadding(11, datebuff), datebuff, getvalue(datalist_i->rx, 11, 1));
		}
		printf(" | %s", getvalue(datalist_i->tx, 11, 1));
		printf(" | %s", getvalue(datalist_i->rx+datalist_i->tx, 11, 1));
		if (cfg.ostyle >= 2) {
			if (datalist_i->next == NULL) {
				d = localtime(&interface->updated);
				printf(" | %s", gettrafficrate(datalist_i->rx+datalist_i->tx, d->tm_sec+(d->tm_min*60)+(d->tm_hour*3600), 14));
			} else {
				printf(" | %s", gettrafficrate(datalist_i->rx+datalist_i->tx, 86400, 14));
			}
		}
		printf("\n");
		if (datalist_i->next == NULL) {
			break;
		}
		datalist_i = datalist_i->next;
	}

	indent(5);
	if (cfg.ostyle >= 2) {
		printf("------------------------+-------------+-------------+---------------\n");
	} else {
		printf("------------------------+-------------+------------\n");
	}

	/* use database update time for estimates */
	d = localtime(&interface->updated);
	if ( datalist_i->rx == 0 || datalist_i->tx == 0 || (d->tm_hour*60+d->tm_min) == 0 ) {
		e_rx = e_tx = 0;
	} else {
		e_rx = ((datalist_i->rx)/(float)(d->tm_hour*60+d->tm_min))*1440;
		e_tx = ((datalist_i->tx)/(float)(d->tm_hour*60+d->tm_min))*1440;
	}

	indent(5);
	printf("estimated   %s", getvalue(e_rx, 11, 2));
	printf(" | %s", getvalue(e_tx, 11, 2));
	printf(" | %s", getvalue(e_rx+e_tx, 11, 2));
	if (cfg.ostyle >= 2) {
		printf(" |\n");
	} else {
		printf("\n");
	}

	dbdatalistfree(&datalist);
}

int showbar(const uint64_t rx, const uint64_t tx, const uint64_t max, const int len)
{
	int i, l, width;

	if ( (rx + tx) < max) {
		width = ( (rx + tx) / (float)max ) * len;
	} else if ((rx + tx) > max) {
		return 0;
	}

	if (len <= 0) {
		return 0;
	}

	printf("  ");

	if (tx > rx) {
		l=rintf((rx/(float)(rx+tx)*width));

		for (i=0; i<l; i++) {
			printf("%c", cfg.rxchar[0]);
		}
		for (i=0; i<(width-l); i++) {
			printf("%c", cfg.txchar[0]);
		}
	} else {
		l=rintf((tx/(float)(rx+tx)*width));
			for (i=0;i<(width-l);i++) {
			printf("%c", cfg.rxchar[0]);
		}
		for (i=0; i<l; i++) {
			printf("%c", cfg.txchar[0]);
		}
	}
	return width;
}

void indent(int i)
{
	if ((cfg.ostyle > 0) && (i > 0)) {
		printf("%*s", i, " ");
	}
}
