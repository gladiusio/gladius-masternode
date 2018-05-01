package masternode

import (
	"github.com/gladiusio/gladius-masternode/internal/networking"
	"github.com/gladiusio/gladius-utils/config"
	"github.com/gladiusio/gladius-utils/init/manager"
)

// Run the Gladius Masternode
func Run() {
	// Setup config handling
	config.SetupConfig("gladius-masternode", config.MasterNodeDefaults())

	// Define some variables
	name, displayName, description :=
		"GladiusMasterNode",
		"Gladius Masternode",
		"Gladius Masternode"

	// Run the function "run" in newtworkd as a service
	manager.RunService(name, displayName, description, networking.StartProxy)
}
