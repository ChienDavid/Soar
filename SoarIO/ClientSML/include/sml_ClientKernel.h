/////////////////////////////////////////////////////////////////
// Kernel class
//
// Author: Douglas Pearson, www.threepenny.net
// Date  : Sept 2004
//
// This class is used by a client app (e.g. an environment) to represent
// the top level connection to the Soar kernel.  You start by creating
// one of these and then creating agents through it etc.
//
/////////////////////////////////////////////////////////////////
#ifndef SML_KERNEL_H
#define SML_KERNEL_H

#include <string>

#include "sml_ObjectMap.h"

namespace sml {

// Forward declarations
class Agent ;
class Connection ;

class Kernel
{
protected:
	long		m_TimeTagCounter ;	// Used to generate time tags (we do them in the kernel not the agent, so ids are unique for all agents)
	long		m_IdCounter ;		// Used to generate unique id names

	Connection*			m_Connection ;
	ObjectMap<Agent*>	m_AgentMap ;
	std::string			m_CommandLineResult;

	// To create a kernel object, use one of the static methods, e.g. Kernel::CreateEmbeddedConnection().
	Kernel(Connection* pConnection);

public:
	/*************************************************************
	* @brief Creates a connection to the Soar kernel that is embedded
	*        within the same process as the caller.
	*
	* @param pLibraryName	The name of the library to load, without an extension (e.g. "KernelSML").  Case-sensitive (to support Linux).
	*						This library will be dynamically loaded and connected to.
	*
	* @returns A new kernel object which is used to communicate with the kernel (or NULL if an error occurs)
	*************************************************************/
	static Kernel* CreateEmbeddedConnection(char const* pLibraryName) ;

	/*************************************************************
	* @brief Creates a connection to a receiver that is in a different
	*        process.  The process can be on the same machine or a different machine.
	*
	* @param pIPaddress The IP address of the remote machine (e.g. "202.55.12.54").
	*                   Pass "127.0.0.1" to create a connection between two processes on the same machine.
	* @param port		The port number to connect to.  The default port for SML is 35353 (picked at random).
	*
	* @returns A new kernel object which is used to communicate with the kernel (or NULL if an error occurs)
	*************************************************************/
	static Kernel* CreateRemoteConnection(char const* pIPaddress, int port) ;

	virtual ~Kernel();

	long	GenerateNextID()		{ return ++m_IdCounter ; }
	long	GenerateNextTimeTag()	{ return --m_TimeTagCounter ; }	// Count down so different from Soar kernel

	/*************************************************************
	* @brief Returns the connection information for this kernel
	*		 which is how we communicate with the kernel (e.g. embedded,
	*		 remotely over a socket etc.)
	*************************************************************/
	Connection* GetConnection() const { return m_Connection ; }

	/*************************************************************
	* @brief Creates a new Soar agent with the given name.
	*
	* @returns A pointer to the agent (or NULL if not found).  This object
	*		   is owned by the kernel and will be destroyed when the
	*		   kernel is destroyed.
	*************************************************************/
	Agent* CreateAgent(char const* pAgentName) ;

	/*************************************************************
	* @brief Looks up an agent by name (from our list of known agents).
	*
	* @returns A pointer to the new agent structure.  This object
	*		   is owned by the kernel and will be destroyed when the
	*		   kernel is destroyed.
	*************************************************************/
	Agent* GetAgent(char const* pAgentName) ;

	/*************************************************************
	* @brief Process a command line command
	*
	* @param pCommandLine Command line string to process.
	* @param pAgentName Agent name to apply the command line to.
	*************************************************************/
	bool ProcessCommandLine(char const* pCommandLine, char const* pAgentName) ;

	/*************************************************************
	* @brief Get last command line result
	*
	* @returns A pointer to the string result.
	*************************************************************/
	const char* GetLastCommandLineResult();
};

}//closes namespace

#endif //SML_KERNEL_H