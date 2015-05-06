## Basic usage ##

### Profile a program ###

To start a profile run for a program, execute:

```
cd aprof-X.Y.Z
./inst/bin/valgrind --tool=aprof your-program [program options]
```
aprof will produce, in the working directory, a report for each thread of your program. Each report is a text file that has a name of the form `command_PID_TID_MEM-RES.aprof` where: `command` is the binary name, `PID` is the process ID, `TID` is the thread ID, and MEM-RES is the [memory resolution](aprof_manual#aprof_command-line_options.md). It contains profile data in [aprof's file format](ReportFileFormat.md). Report files can be visualized with [aprof-plot](Aprofplot.md).

### Visualize profile reports ###

To analyze the profile report, start aprof-plot as follows:

  * If you have downloaded aprof-plot from our download section, you can run it in this way:
```
cd aprof-plot-X.Y.Z
java -jar aprof-plot.jar
```

  * If you have downloaded aprof-plot from our SVN, then you can run the built jar in this way:
```
cd aprofplot
ant run
```