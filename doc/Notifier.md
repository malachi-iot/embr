# Notifier

TBD: Move some of the Services.md descriptions into here and link them

## Properties

## Macros

### EMBR_PROPERTIES_BEGIN / EMBR_PROPERTIES_END

Can't remember if this is the original outmoded one or the next gen better one.

Creates a submordinate 'id' class with instance named 'fields_'

### EMBR_PROPERTY_RUNTIME_BEGIN / EMBR_PROPERTY_RUNTIME_END

Takes base_ parameter denoting base Service/Property class providing
runtime.  A bit confusing, DEBT

### EMBR_PROPERTIES_SPARSE_BEGIN / EMBR_PROPERTIES_SPARSE_END

Operates without creating a suborinate/helper instance

### EMBR_PROPERTY

Happily sits freestanding in a service runtime

### EMBR_PROPERTY_ALIAS

### EMBR_PROPERTY_ID

Calls EMBR_PROPERTY_ID2_2

### EMBR_PROPERTY_ID_ALIAS

### EMBR_PROPERTY_ID_EXT

Seems to have numeric/enum style ID associated with property

### EMBR_PROPERTY_ID_LOOKUP

### EMBR_PROPERTY_ID2_2

Low level property declaration macro.  Sets up 'name' struct via EMBR_PROPERTY_ID2_2 and also calls EMBR_PROPERTY_ID_LOOKUP

### EMBR_PROPERTY_ID2_BASE

Low level property declaration macro.  Creates a struct whose name is that
of the property.  Said struct contains:

* get accessor
* set mutator
* store(this_type) convenience accessor to pull fields_ instance out

Relies on fields_ instance from EMBR_PROPERTIES_BEGIN

### EMBR_INTERNAL_PROPERTY_ALIAS

Creates accessor & mutator:

* type alias() { return base::getter<impl::id::name>}
* void alias(type) { return base::setter<impl::id::name>}

More or less a helper to get at the id:: properties

### EMBR_SERVICE_RUNTIME_START / EMBR_SERVICE_RUNTIME_END
