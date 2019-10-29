
/*
 H2O - Convert Harvest csv files into Odoo format
 */

/*
 * history
 * 22.10.2015 - properly replace decimal point to accept >=10 hours
 * 27.10.2016 - add notes to task description
 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

#define SIZE	1024
#define PCOUNT	19

void error_exit (string msg);
void error_exit (fstream& f, string msg);
void error_exit (fstream& f, fstream& g, string msg, int line);

const char* pmap[2*PCOUNT] = {
	"__export__.account_analytic_account_29","Tic",
	"__export__.account_analytic_account_30","Ayt",
	"__export__.account_analytic_account_31","Cer",
	"__export__.account_analytic_account_32","Sim",
	"__export__.account_analytic_account_33","UvA",
	"__export__.account_analytic_account_34","Nor",
	"__export__.account_analytic_account_40","Par",
	"__export__.account_analytic_account_44","PwC",
	"__export__.account_analytic_account_55","Bar",
	"__export__.account_analytic_account_56","XPO",
	"__export__.account_analytic_account_76","ZES",
	"__export__.account_analytic_account_73","Int",
	"__export__.account_analytic_account_72","Cir",
	"__export__.account_analytic_account_83","Gai",
	"__export__.account_analytic_account_87","Web",
	"__export__.account_analytic_account_88","Com",
	"__export__.account_analytic_account_113","Qua",
	"__export__.account_analytic_account_120","Sta",
    "__export__.account_analytic_account_153","Fea"
};

int lookup_project (int code)
{
	int i;
	for (i = 0; i < PCOUNT; i++)
		if (code == tolower(pmap[2*i+1][0])+256*tolower(pmap[2*i+1][1])
					+256*256*tolower(pmap[2*i+1][2]))
			return 2*i;
	return -1;
}

int main (int argc, char **argv)
{
	fstream f, g;
	char buf[SIZE], outfname[SIZE];
	int i = 0;
	// check args
	if (argc != 2)
		error_exit ("Need input file");
	// open input file
	f.open (argv[1], fstream::in);
	if (f.fail ())
		error_exit ("Error opening input file");
	// skip column header
	f.getline (buf, SIZE);
	if (f.fail ())
		error_exit (f, "Read fail at column header");
	// open output file
	snprintf (outfname, sizeof (outfname), "%s%s", "out-", argv[1]);
	g.open (outfname, fstream::out);
	if (g.fail ())
		error_exit (f, "Error opening output file");
	// write output column header
	g << "date,account_id/id,journal_id/id,name,unit_amount" << endl;
	// parse lines
	while (++i && f.getline (buf, SIZE) && !f.eof ())
	{
		istringstream linein (buf);
		ostringstream lineout;
		if (f.fail () || linein.fail ())
			error_exit (f, g, "Read fail at line ", i);
		// ignore leading whitespaces, skip empty lines
		if ((linein >> ws).peek () == EOF)
			continue;
		// read date in yyyy/mm/dd format
		{
			int dd, mm, yyyy;
			char c, date[11];
			linein >> yyyy >> c >> mm >> c >> dd >> c;
			if (linein.fail ())
				error_exit (f, g, "Fail to read date at line ", i);
			snprintf (date, sizeof (date), "%4d-%02d-%02d", yyyy, mm, dd);
			lineout << date << ",";
			if (lineout.fail ())
				error_exit (f, g, "Fail to write date at line ", i);
		}
		// read client
		{
			char cl[SIZE];
			linein.getline (cl, SIZE, ',');
			if (linein.fail ())
				error_exit (f, g, "Fail to read project name at line ", i);
			int pid =
				lookup_project (tolower(cl[0])+256*tolower(cl[1])+256*256*tolower(cl[2]));
			if (pid == -1)
				error_exit (f, g, "Project not fount at line ", i);
			lineout << pmap[pid] << ",hr_timesheet.analytic_journal,";
			if (lineout.fail ())
				error_exit (f, g, "Fail to write internal project name at line ", i);
		}
		// skip project and project code
		linein.ignore (SIZE, ',');
		linein.ignore (SIZE, ',');
		if (linein.fail ())
			error_exit (f, g, "Fail to skip project or project code at line ", i);
		// read task
		{
			char task[SIZE];
			linein.getline (task, SIZE, ',');
			if (linein.fail ())
				error_exit (f, g, "Fail to read task at line ", i);
			lineout << task;
			if (lineout.fail ())
				error_exit (f, g, "Fail to write task at line ", i);
		}
		// read notes
		{
			char c = 0, notes[SIZE] = {0};
			int j = 0;
			if (linein.peek () == 34)
			{
				linein.get ();
				while ((c = linein.get ()) != 34)
				{
					if (j == SIZE)
						error_exit (f, g, "Buffer size exceeded reading notes at line ", i);
					if (c == EOF)
					{
						f.getline (buf, SIZE);
						if (f.eof ()) {
							error_exit (f, g, "Unexpected end of file reading notes at line ", i);
						}
						linein.clear ();
						linein.str (buf);
						notes[j++] = ';';
					}
					else if (c != ',')
						notes[j++] = c;
				}
			}
			linein.ignore (SIZE, ',');
			if (linein.fail ())
				error_exit (f, g, "Fail to read notes at line ", i);
			if (notes[0])
				lineout << ": " << notes;
			lineout << ",";
		}
		// read hours as a decimal string
		{
			char hrs[SIZE];
			linein.ignore (1);
			linein.getline (hrs, SIZE, '\"');
			if (linein.fail ())
				error_exit (f, g, "Fail to read hours at line ", i);
			// replace , with .
			for (char *p = hrs;*p;p++)
				if (*p == ',') *p = '.';
			lineout << hrs << endl;
			if (lineout.fail ())
				error_exit (f, g, "Fail to write hours at line ", i);
		}
		// skip everything else
		g << lineout.str();
		if (g.fail ())
			error_exit (f, g, "Fail to write output at line ", i);
	}
	f.close ();
	g.close ();
	cout << "Output written in " << outfname << endl;
	return 0;
}

void error_exit (string msg)
{
	cerr << msg.c_str() << endl;
	exit (0);
}

void error_exit (fstream& f, string msg)
{
	f.close ();
	cerr << msg.c_str() << endl;
	exit (0);
}

void error_exit (fstream& f, fstream& g, string msg, int line)
{
	char buf[SIZE];
	snprintf (buf, sizeof (buf), "%s%d", msg.c_str(), line);
	f.close();
	g.close();
	cerr << buf << endl;
	exit (0);
}
