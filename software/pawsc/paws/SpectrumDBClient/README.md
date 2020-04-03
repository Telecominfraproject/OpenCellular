
# Introduction
This is a client which provides LTE access, via a small/femto cell, by communicating with a TVWSDB to negotiate a spectrum.   There are two main applications which, together, accomplish this : PAWS and SCTP-AGENT.

## PAWS
This is the application which communicates with an Ofcom-certified TVWS database.  The user will need to configure details on the various devices for this to be achieved.  
Referer to RFC 7545 for more details.

## SCTP-AGENT 
This is an application which controls access to the MME itself.   It is used to control the transfer of data between the local small-cell eNodeB LTE software, and the MME itself.
It uses the information from PAWS to determine whether it needs to block the data transfer.  However it can also be configured to ignore the PAWS status.
The block diagrams below shows the general topologies.
 
![](https://github.com/Microsoft/SpectrumDBClient/blob/master/topology.JPG)

# TVWSDB
As discussed, PAWS needs to communicate with a TVWSDB.  The user will need to register all clients with this, get security tokens etc, and configure this in the SQL databases mentioned below.

# Build instructions

## Set environment variable
> export PRJROOT="root-dir-containing-apps-and-libs"

## SCTP-AGENT
### Config options
There are some build options in apps/sctp-agent/sctp_cfg_options.h.   Change these as required.
### Set up real "tcps1", or use dummy.
+ The SCTP-agent can communicate in either SCTP ot TCP mode, by setting "SocketInfo.mme_connection_type" to "tcp|sctp".
+ If you run in SCTP mode, you need to create a soft-link in the apps/sctp-agent directory to point to dummy_tcps1
> cd $PRJROOT/apps/sctp-agent
> ln -s dummy_tcps1 tcps1
+ NOTE that the dummy_tcps1 is purely used to allow the code to compile.  It does not provide a TCP-based access
+ If you run in TCP mode, you need to use the tcps1 library, and create a softlink in the apps/sctp-agent directory to point to the directory where you have cloned tcps1lib.
> cd $PRJROOT/apps/sctp-agent
> ln -s <tcps1lib-directory> tcps1


## Compile code
> cd $PRJROOT
> make TARGET_PLATFORM=native|fap clean
> make TARGET_PLATFORM=native|fap 


## Databases (PAWS/SCTP)
+ Both the PAWS and SCTP agent are configured using an SQL database.
+ The schema for each of these can be found on the "sql_db" subdirectory of each of these apps. 
+ Furthermore "<app>_native" in each of these directories gives a full example.
+ To generate different databases, update the "json" files in these directories and run the Makefile with TARGET_PLATFORM="native|fap"
+ Note that the databases are not automatically built by a "make" at the top level.


## Databases (FemTo)
This whole small cell client assumes that there is also a "femto.db" that can be read.


## Database locations
The location where these databases are found in hardcoded.  To change, update the following files accordingly :-
### PAWS
> apps/paws/fap/main.c
>apps/paws/native/main.c
### SCTP-AGENT
> apps/sctp-agent/fap/device_db_info.h
> apps/sctp-agent/native/device_db_info.h

## TARGET_PLATFORM
+ You will see above the TARGET_PLATFORM should be set to "native|fap" when running "make".  In this case, "native" means x86 linux.
+ For "fap" the user would need to setup their environment as required. 


## Contributions
Contributions in the form of bug reports, feature requests and PRs are always welcome.

Please follow these steps before submitting a PR:
* Create an issue describing the bug or feature request.
* Clone the repository and create a topic branch.
* Make changes, adding new tests for new functionality.
* Submit a PR.

## License
See [LICENSE](LICENSE).

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

