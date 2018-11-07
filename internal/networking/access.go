package networking

// AccessStrategy is an interface for specific caching strategies to
// implement. This allows us some flexibility with which kind of caching
// mechanism / system we can use without having to worry about the implementation
// in calling code. i.e. we can use blob storage, a seed node, file system,
// etc. interchangeably.
type AccessStrategy interface {
	Retrieve() // Get an item from the cache
	Store()    // Add an item to the cache
}

// SeedNodeAccessStrategy is an implementation of AccessStrategy that
// will handle the details of adding and retrieving content from a
// seed node.
type SeedNodeAccessStrategy struct {
	node *NetworkNode
}

// Add more AccessStrategy's here if you wish...
