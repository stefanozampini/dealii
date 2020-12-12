// ---------------------------------------------------------------------
//
// Copyright (C) 2017 - 2020 by the deal.II authors
//
// This file is part of the deal.II library.
//
// The deal.II library is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE.md at
// the top level directory of deal.II.
//
// ---------------------------------------------------------------------

#ifndef dealii_particles_particle_h
#define dealii_particles_particle_h

#include <deal.II/base/config.h>

#include <deal.II/base/array_view.h>
#include <deal.II/base/point.h>
#include <deal.II/base/types.h>

#include <deal.II/particles/property_pool.h>

#include <cstdint>

DEAL_II_NAMESPACE_OPEN

namespace types
{
  /* Type definitions */

#ifdef DEAL_II_WITH_64BIT_INDICES
  /**
   * The type used for indices of particles. While in
   * sequential computations the 4 billion indices of 32-bit unsigned integers
   * is plenty, parallel computations using hundreds of processes can overflow
   * this number and we need a bigger index space. We here utilize the same
   * build variable that controls the dof indices because the number
   * of degrees of freedom and the number of particles are typically on the same
   * order of magnitude.
   *
   * The data type always indicates an unsigned integer type.
   */
  using particle_index = uint64_t;

#  ifdef DEAL_II_WITH_MPI
  /**
   * An identifier that denotes the MPI type associated with
   * types::global_dof_index.
   */
#    define DEAL_II_PARTICLE_INDEX_MPI_TYPE MPI_UINT64_T
#  endif

#else
  /**
   * The type used for indices of particles. While in
   * sequential computations the 4 billion indices of 32-bit unsigned integers
   * is plenty, parallel computations using hundreds of processes can overflow
   * this number and we need a bigger index space. We here utilize the same
   * build variable that controls the dof indices because the number
   * of degrees of freedom and the number of particles are typically on the same
   * order of magnitude.
   *
   * The data type always indicates an unsigned integer type.
   */
  using particle_index = unsigned int;

#  ifdef DEAL_II_WITH_MPI
  /**
   * An identifier that denotes the MPI type associated with
   * types::global_dof_index.
   */
#    define DEAL_II_PARTICLE_INDEX_MPI_TYPE MPI_UNSIGNED
#  endif
#endif
} // namespace types

/**
 * A namespace that contains all classes that are related to the particle
 * implementation, in particular the fundamental Particle class.
 */
namespace Particles
{
  namespace internal
  {
    /**
     * Internal alias of cell level/index pair.
     */
    using LevelInd = std::pair<int, int>;
  } // namespace internal

  /**
   * A class that represents a particle in a domain that is meshed by
   * a triangulation of some kind. The data this class stores is the
   * position of the particle in the overall space, the position of
   * the particle in the reference coordinate system of the cell it is
   * currently in, an ID number that is unique among all particles,
   * and a variable number of "properties".
   *
   * The "properties" attached to each object of this class are
   * stored by a PropertyPool object. These properties are
   * stored as an array of `double` variables that can be accessed
   * via an ArrayView object. For example, if one wanted to equip
   * each particle with a "temperature" and "chemical composition"
   * property that is advected along with the particle (and may change
   * from time step to time step based on some differential equation,
   * for example), then one would allocate two properties per particle
   * in the PropertyPool object.
   *
   * In practice, however, one often wants to associate properties
   * with particles that are not just independent numbers as in the
   * situation above. An example would be if one wanted to track the
   * stress or strain that a particle is subjected to -- a tensor-valued
   * quantity. In these cases, one would <i>interpret</i> these scalar
   * properties as the <i>components of the stress or strain</i>. In
   * other words, one would first tell the PropertyPool to allocate
   * as many properties per particle as there are components in the
   * tensor one wants to track, and then write small conversion functions that
   * take the ArrayView of scalar properties returned by the
   * get_properties() function and convert it to a tensor of the
   * appropriate type. This can then be evaluated and evolved in each
   * time step. A second conversion function would convert back from a
   * tensor to an ArrayView object to store the updated data back in the
   * particle via the set_properties() function.
   *
   * There are of course cases where the properties one cares about are
   * not real (or, in computers, floating point) numbers but rather
   * categorical: For example, one may want to mark some particles
   * as "red", "blue", or "green". The property might then either be
   * represented as an integer, or as an element of an `enum`. In these
   * cases, one would need to come up with a way to <i>represent</i>
   * these sorts of categorical fields in terms of floating point
   * numbers. For example, one could map "red" to the floating point number
   * 1.0, "blue" to 2.0, and "green" to 3.0. The conversion functions
   * to translate between these two representations should then not be very
   * difficult to write either.
   *
   * @ingroup Particle
   */
  template <int dim, int spacedim = dim>
  class Particle
  {
  public:
    /**
     * Empty constructor for Particle, creates a particle at the
     * origin.
     */
    Particle();

    /**
     * Constructor for Particle, creates a particle with the specified
     * ID at the specified location. Note that there is no
     * check for duplicate particle IDs so the user must
     * make sure the IDs are unique over all processes.
     *
     * @param[in] location Initial location of particle.
     * @param[in] reference_location Initial location of the particle
     * in the coordinate system of the reference cell.
     * @param[in] id Globally unique ID number of particle.
     */
    Particle(const Point<spacedim> &     location,
             const Point<dim> &          reference_location,
             const types::particle_index id);

    /**
     * Copy-Constructor for Particle, creates a particle with exactly the
     * state of the input argument. Note that since each particle has a
     * handle for a certain piece of the property memory, and is responsible
     * for registering and freeing this memory in the property pool this
     * constructor registers a new chunk, and copies the properties.
     */
    Particle(const Particle<dim, spacedim> &particle);

    /**
     * Constructor for Particle, creates a particle from a data vector.
     * This constructor is usually called after serializing a particle by
     * calling the write_data() function.
     *
     * @param[in,out] begin_data A pointer to a memory location from which
     * to read the information that completely describes a particle. This
     * class then de-serializes its data from this memory location and
     * advance the pointer accordingly.
     *
     * @param[in,out] property_pool An optional pointer to a property pool
     * that is used to manage the property data used by this particle. Note that
     * if a non-null pointer is handed over this constructor assumes @p begin_data
     * contains serialized data of the same length and type that is allocated
     * by @p property_pool.
     */
    Particle(const void *&                      begin_data,
             PropertyPool<dim, spacedim> *const property_pool = nullptr);

    /**
     * Move constructor for Particle, creates a particle from an existing
     * one by stealing its state.
     */
    Particle(Particle<dim, spacedim> &&particle) noexcept;

    /**
     * Copy assignment operator.
     */
    Particle<dim, spacedim> &
    operator=(const Particle<dim, spacedim> &particle);

    /**
     * Move assignment operator.
     */
    Particle<dim, spacedim> &
    operator=(Particle<dim, spacedim> &&particle) noexcept;

    /**
     * Destructor. Releases the property handle if it is valid, and
     * therefore frees that memory space for other particles. (Note:
     * the memory is managed by the property pool, and the pool is responsible
     * for what happens to the memory.
     */
    ~Particle();

    /**
     * Write particle data into a data array. The array is expected
     * to be large enough to take the data, and the void pointer should
     * point to the first entry of the array to which the data should be
     * written. This function is meant for serializing all particle properties
     * and later de-serializing the properties by calling the appropriate
     * constructor Particle(void *&data, PropertyPool *property_pool = nullptr);
     *
     * @param [in,out] data The memory location to write particle data
     * into. This pointer points to the begin of the memory, in which the
     * data will be written and it will be advanced by the serialized size
     * of this particle.
     */
    void
    write_data(void *&data) const;


    /**
     * Update all of the data associated with a particle : id,
     * location, reference location and, if any, properties by using a
     * data array. The array is expected to be large enough to take the data,
     * and the void pointer should point to the first entry of the array to
     * which the data should be written. This function is meant for
     * de-serializing the particle data without requiring that a new Particle
     * class be built. This is used in the ParticleHandler to update the
     * ghost particles without de-allocating and re-allocating memory.
     *
     * @param[in,out] data A pointer to a memory location from which
     * to read the information that completely describes a particle. This
     * class then de-serializes its data from this memory location and
     * advance the pointer accordingly.
     */
    void
    update_particle_data(const void *&data);

    /**
     * Set the location of this particle. Note that this does not check
     * whether this is a valid location in the simulation domain.
     *
     * @param [in] new_location The new location for this particle.
     *
     * @note In parallel programs, the ParticleHandler class stores particles
     *   on both the locally owned cells, as well as on ghost cells. The
     *   particles on the latter are *copies* of particles owned on other
     *   processors, and should therefore be treated in the same way as
     *   ghost entries in @ref GlossGhostedVector "vectors with ghost elements"
     *   or @ref GlossGhostCell "ghost cells": In both cases, one should
     *   treat the ghost elements or cells as `const` objects that shouldn't
     *   be modified even if the objects allow for calls that modify
     *   properties. Rather, properties should only be modified on processors
     *   that actually *own* the particle.
     */
    void
    set_location(const Point<spacedim> &new_location);

    /**
     * Get the location of this particle.
     *
     * @return The location of this particle.
     */
    const Point<spacedim> &
    get_location() const;

    /**
     * Set the reference location of this particle.
     *
     * @param [in] new_reference_location The new reference location for
     * this particle.
     *
     * @note In parallel programs, the ParticleHandler class stores particles
     *   on both the locally owned cells, as well as on ghost cells. The
     *   particles on the latter are *copies* of particles owned on other
     *   processors, and should therefore be treated in the same way as
     *   ghost entries in @ref GlossGhostedVector "vectors with ghost elements"
     *   or @ref GlossGhostCell "ghost cells": In both cases, one should
     *   treat the ghost elements or cells as `const` objects that shouldn't
     *   be modified even if the objects allow for calls that modify
     *   properties. Rather, properties should only be modified on processors
     *   that actually *own* the particle.
     */
    void
    set_reference_location(const Point<dim> &new_reference_location);

    /**
     * Return the reference location of this particle in its current cell.
     */
    const Point<dim> &
    get_reference_location() const;

    /**
     * Return the ID number of this particle. The ID of a particle is intended
     * to be a property that is globally unique even in parallel computations
     * and is transfered along with other properties of a particle if it
     * moves from a cell owned by the current processor to a cell owned by
     * a different processor, or if ownership of the cell it is on is
     * transferred to a different processor.
     */
    types::particle_index
    get_id() const;

    /**
     * Set the ID number of this particle. The ID of a particle is intended
     * to be a property that is globally unique even in parallel computations
     * and is transfered along with other properties of a particle if it
     * moves from a cell owned by the current processor to a cell owned by
     * a different processor, or if ownership of the cell it is on is
     * transferred to a different processor. As a consequence, when setting
     * the ID of a particle, care needs to be taken to ensure that particles
     * have globally unique IDs. (The ParticleHandler does not itself check
     * whether particle IDs so set are globally unique in a parallel setting
     * since this would be a very expensive operation.)
     *
     * @param[in] new_id The new ID number for this particle.
     *
     * @note In parallel programs, the ParticleHandler class stores particles
     *   on both the locally owned cells, as well as on ghost cells. The
     *   particles on the latter are *copies* of particles owned on other
     *   processors, and should therefore be treated in the same way as
     *   ghost entries in @ref GlossGhostedVector "vectors with ghost elements"
     *   or @ref GlossGhostCell "ghost cells": In both cases, one should
     *   treat the ghost elements or cells as `const` objects that shouldn't
     *   be modified even if the objects allow for calls that modify
     *   properties. Rather, properties should only be modified on processors
     *   that actually *own* the particle.
     */
    void
    set_id(const types::particle_index &new_id);

    /**
     * Tell the particle where to store its properties (even if it does not
     * own properties). Usually this is only done once per particle, but
     * since the particle does not know about the properties,
     * we want to do it not at construction time. Another use for this
     * function is after particle transfer to a new process.
     *
     * If a particle already stores properties in a property pool, then
     * their values are saved, the memory is released in the previous
     * property pool, and a copy of the particle's properties will be
     * allocated in the new property pool.
     */
    void
    set_property_pool(PropertyPool<dim, spacedim> &property_pool);

    /**
     * Return whether this particle has a valid property pool and a valid
     * handle to properties.
     */
    bool
    has_properties() const;

    /**
     * Set the properties of this particle.
     *
     * @param [in] new_properties An ArrayView containing the
     * new properties for this particle.
     *
     * @note In parallel programs, the ParticleHandler class stores particles
     *   on both the locally owned cells, as well as on ghost cells. The
     *   particles on the latter are *copies* of particles owned on other
     *   processors, and should therefore be treated in the same way as
     *   ghost entries in @ref GlossGhostedVector "vectors with ghost elements"
     *   or @ref GlossGhostCell "ghost cells": In both cases, one should
     *   treat the ghost elements or cells as `const` objects that shouldn't
     *   be modified even if the objects allow for calls that modify
     *   properties. Rather, properties should only be modified on processors
     *   that actually *own* the particle.
     */
    void
    set_properties(const ArrayView<const double> &new_properties);

    /**
     * Get write-access to properties of this particle. If the
     * particle has no properties yet, but has access to a
     * PropertyPool object it will allocate properties to
     * allow writing into them. If it has no properties and
     * has no access to a PropertyPool this function will
     * throw an exception.
     *
     * @return An ArrayView of the properties of this particle.
     */
    const ArrayView<double>
    get_properties();

    /**
     * Get read-access to properties of this particle. If the particle
     * has no properties this function throws an exception.
     *
     * @return An ArrayView of the properties of this particle.
     */
    const ArrayView<const double>
    get_properties() const;

    /**
     * Return the size in bytes this particle occupies if all of its data is
     * serialized (i.e. the number of bytes that is written by the write_data
     * function of this class).
     */
    std::size_t
    serialized_size_in_bytes() const;

    /**
     * Write the data of this object to a stream for the purpose of
     * serialization.
     */
    template <class Archive>
    void
    save(Archive &ar, const unsigned int version) const;

    /**
     * Read the data of this object from a stream for the purpose of
     * serialization. Note that in order to store the properties
     * correctly, the property pool of this particle has to
     * be known at the time of reading, i.e. set_property_pool()
     * has to have been called, before this function is called.
     */
    template <class Archive>
    void
    load(Archive &ar, const unsigned int version);

    /**
     * Free the memory of the property pool
     */
    void
    free_properties();

#ifdef DOXYGEN
    /**
     * Write and read the data of this object from a stream for the purpose
     * of serialization.
     */
    template <class Archive>
    void
    serialize(Archive &archive, const unsigned int version);
#else
    // This macro defines the serialize() method that is compatible with
    // the templated save() and load() method that have been implemented.
    BOOST_SERIALIZATION_SPLIT_MEMBER()
#endif

  private:
    /**
     * Current particle location.
     */
    Point<spacedim> location;

    /**
     * Current particle location in the reference cell.
     */
    Point<dim> reference_location;

    /**
     * Globally unique ID of particle.
     */
    types::particle_index id;

    /**
     * A pointer to the property pool. Necessary to translate from the
     * handle to the actual memory locations.
     */
    PropertyPool<dim, spacedim> *property_pool;

    /**
     * A handle to all particle properties
     */
    typename PropertyPool<dim, spacedim>::Handle properties;
  };

  /* ---------------------- inline and template functions ------------------ */

  template <int dim, int spacedim>
  template <class Archive>
  inline void
  Particle<dim, spacedim>::load(Archive &ar, const unsigned int)
  {
    unsigned int n_properties = 0;

    ar &location &reference_location &id &n_properties;

    if (n_properties > 0)
      {
        ArrayView<double> properties(get_properties());
        Assert(
          properties.size() == n_properties,
          ExcMessage(
            "This particle was serialized with " +
            std::to_string(n_properties) +
            " properties, but the new property handler provides space for " +
            std::to_string(properties.size()) +
            " properties. Deserializing a particle only works for matching property sizes."));

        ar &boost::serialization::make_array(get_properties().data(),
                                             n_properties);
      }
  }



  template <int dim, int spacedim>
  template <class Archive>
  inline void
  Particle<dim, spacedim>::save(Archive &ar, const unsigned int) const
  {
    unsigned int n_properties = 0;
    if ((property_pool != nullptr) &&
        (properties != PropertyPool<dim, spacedim>::invalid_handle))
      n_properties = get_properties().size();

    ar &location &reference_location &id &n_properties;

    if (n_properties > 0)
      ar &boost::serialization::make_array(get_properties().data(),
                                           n_properties);
  }



  template <int dim, int spacedim>
  inline void
  Particle<dim, spacedim>::set_location(const Point<spacedim> &new_loc)
  {
    location = new_loc;
  }



  template <int dim, int spacedim>
  inline const Point<spacedim> &
  Particle<dim, spacedim>::get_location() const
  {
    return location;
  }



  template <int dim, int spacedim>
  inline void
  Particle<dim, spacedim>::set_reference_location(const Point<dim> &new_loc)
  {
    reference_location = new_loc;
  }



  template <int dim, int spacedim>
  inline const Point<dim> &
  Particle<dim, spacedim>::get_reference_location() const
  {
    return reference_location;
  }



  template <int dim, int spacedim>
  inline types::particle_index
  Particle<dim, spacedim>::get_id() const
  {
    return id;
  }



  template <int dim, int spacedim>
  inline void
  Particle<dim, spacedim>::set_id(const types::particle_index &new_id)
  {
    id = new_id;
  }



  template <int dim, int spacedim>
  inline void
  Particle<dim, spacedim>::set_property_pool(
    PropertyPool<dim, spacedim> &new_property_pool)
  {
    // First, we do want to save any properties that may
    // have previously been set, and copy them over to the memory allocated
    // on the new pool
    typename PropertyPool<dim, spacedim>::Handle new_handle =
      PropertyPool<dim, spacedim>::invalid_handle;
    if (property_pool != nullptr &&
        properties != PropertyPool<dim, spacedim>::invalid_handle)
      {
        new_handle = new_property_pool.allocate_properties_array();

        ArrayView<double> old_properties = this->get_properties();
        ArrayView<double> new_properties =
          new_property_pool.get_properties(new_handle);
        std::copy(old_properties.cbegin(),
                  old_properties.cend(),
                  new_properties.begin());
      }

    // If the particle currently has a reference to properties, then
    // release those.
    if (property_pool != nullptr &&
        properties != PropertyPool<dim, spacedim>::invalid_handle)
      property_pool->deallocate_properties_array(properties);


    // Then set the pointer to the property pool we want to use. Also set the
    // handle to any properties, if we have copied any above.
    property_pool = &new_property_pool;
    properties    = new_handle;
  }



  template <int dim, int spacedim>
  inline const ArrayView<const double>
  Particle<dim, spacedim>::get_properties() const
  {
    Assert(has_properties(), ExcInternalError());

    return property_pool->get_properties(properties);
  }



  template <int dim, int spacedim>
  inline bool
  Particle<dim, spacedim>::has_properties() const
  {
    return (property_pool != nullptr) &&
           (properties != PropertyPool<dim, spacedim>::invalid_handle);
  }

} // namespace Particles

DEAL_II_NAMESPACE_CLOSE


namespace boost
{
  namespace geometry
  {
    namespace index
    {
      // Forward declaration of bgi::indexable
      template <class T>
      struct indexable;

      /**
       * Make sure we can construct an RTree of Particles::Particle objects.
       */
      template <int dim, int spacedim>
      struct indexable<dealii::Particles::Particle<dim, spacedim>>
      {
        /**
         * boost::rtree expects a const reference to an indexable object. For
         * a Particles::Particle object, this is its reference location.
         */
        using result_type = const dealii::Point<spacedim> &;

        result_type
        operator()(
          const dealii::Particles::Particle<dim, spacedim> &particle) const
        {
          return particle.get_location();
        }
      };

    } // namespace index
  }   // namespace geometry
} // namespace boost

#endif
