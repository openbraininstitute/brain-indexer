Conventions
===========

In order to use brain-indexer with other neurscientific tools certain conventions
need to be followed. This page details these assumptions for synapse and
morphology indexes.

Morphology Conventions
----------------------

This page describes the conventions used for defining the radius and the IDs in
morphology indexes.

The preferred format for morphologies is SONATA. In SONATA a morphology is
stored as a soma, dendrites and an axon. The neurites are each stored as a
collection of sections. Each section is an unbraching part of the dendrite or
axon. A section consists of a sequence of points along the center line of the
section and the radius of the section at those locations.

A natural interpretation of these discrete values is as piecewise linear center
lines with a piecewise linear radius. However, in brain-indexer we only deal with
cylinders. The **radius** of the cylinder is the average of the radii at each
endpoint.

The element of a morphology are identified by three numbers:

* the GID which defines the neuron to which the element belongs,
* the section ID within that neuron,
* and the segment ID with the section.

The GIDs are zero-based, and therefore consistent with SONATA. Within the BBP
ecosystem some programs have one-based GIDs. Hence care is needed when
interacting with other tools.

The soma always has section ID zero. Sections always start from section ID one.
The order in which the sections are numbered is consistent with BluePy and
MorphIO. Furthermore, consistent with both BluePy and MorphIO, uniforcations
have been removed. Meaning each section contains all segments between to
neighbouring branching points; and branching points always join three or more
sections.

Synapse Conventions
-------------------

This section describes the conventions used for defining the shape and the ID in
synapse indexes.

The shape of a synapse is a point. The point is the ``afferent_center_*`` as
defined in SONATA edge files. The IDs follow the SONATA conventions.
