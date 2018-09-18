package masternode

import (
	"github.com/gladiusio/gladius-masternode/config"
	"github.com/gladiusio/gladius-masternode/internal/networking"
	gconfig "github.com/gladiusio/gladius-utils/config"
	"github.com/gladiusio/gladius-utils/init/manager"
	log "github.com/sirupsen/logrus"
)

// Run the Gladius Masternode
func Run() {
	base, err := gconfig.GetGladiusBase()
	if err != nil {
		log.WithFields(log.Fields{
			"err": err,
		}).Fatal("Couldn't get Gladius base")
	}
	// Setup config handling
	config.SetupConfig(base)

	// Define some variables
	name, displayName, description :=
		"GladiusMasterNode",
		"Gladius Masternode",
		"Gladius Masternode"

	// Run the function "run" in networking package as a service
	manager.RunService(name, displayName, description, networking.StartProxy)
}
