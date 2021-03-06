/****************************************************************/
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*          All contents are licensed under LGPL V2.1           */
/*             See LICENSE for full restrictions                */
/****************************************************************/

#include "ConservedNoiseBase.h"

// libmesh includes
#include "libmesh/quadrature.h"

template<>
InputParameters validParams<ConservedNoiseBase>()
{
  InputParameters params = validParams<ElementUserObject>();

  MultiMooseEnum setup_options(SetupInterface::getExecuteOptions());
  setup_options = "timestep_begin";
  params.set<MultiMooseEnum>("execute_on") = setup_options;
  return params;
}

ConservedNoiseBase::ConservedNoiseBase(const InputParameters & parameters) :
    ConservedNoiseInterface(parameters)
{
}

void
ConservedNoiseBase::initialize()
{
  _random_data.clear();
  _integral = 0.0;
  _volume = 0.0;
}

void
ConservedNoiseBase::execute()
{
  // reserve space for each quadrature point in the element
  std::vector<Real> & me = _random_data[_current_elem->id()] = std::vector<Real>(_qrule->n_points());

  // store a random number for each quadrature point
  for (_qp=0; _qp<_qrule->n_points(); _qp++)
  {
    me[_qp] = getQpRandom();
    _integral += _JxW[_qp] * _coord[_qp] * me[_qp];
    _volume   += _JxW[_qp] * _coord[_qp];
  }
}

void
ConservedNoiseBase::threadJoin(const UserObject &y)
{
  const ConservedNoiseBase & uo = static_cast<const ConservedNoiseBase &>(y);

  _random_data.insert(uo._random_data.begin(), uo._random_data.end());
  _integral += uo._integral;
  _volume += uo._volume;
}

void
ConservedNoiseBase::finalize()
{
  gatherSum(_integral);
  gatherSum(_volume);

  _offset = _integral / _volume;
}

Real
ConservedNoiseBase::getQpValue(dof_id_type element_id, unsigned int qp) const
{
  LIBMESH_BEST_UNORDERED_MAP<dof_id_type, std::vector<Real> >::const_iterator me = _random_data.find(element_id);

  if (me == _random_data.end())
    mooseError("Element not found.");
  else
  {
    libmesh_assert_less(qp, me->second.size());
    return me->second[qp] - _offset;
  }
}

