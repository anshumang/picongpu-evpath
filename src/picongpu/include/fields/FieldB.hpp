/**
 * Copyright 2013-2014 Axel Huebl, Heiko Burau, Rene Widera, Richard Pausch
 *
 * This file is part of PIConGPU.
 *
 * PIConGPU is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PIConGPU is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PIConGPU.
 * If not, see <http://www.gnu.org/licenses/>.
 */



#pragma once

#include <string>

/*pic default*/
#include "types.h"
#include "simulation_defines.hpp"
#include "simulation_classTypes.hpp"

#include "Fields.def"
#include "fields/SimulationFieldHelper.hpp"
#include "dataManagement/ISimulationData.hpp"

/*libPMacc*/
#include "memory/buffers/GridBuffer.hpp"
#include "mappings/simulation/GridController.hpp"
#include "fields/LaserPhysics.hpp"
#include "memory/boxes/DataBox.hpp"
#include "memory/boxes/PitchedBox.hpp"

#include "math/Vector.hpp"


namespace picongpu
{
    using namespace PMacc;

    class FieldB : public SimulationFieldHelper<MappingDesc>, public ISimulationData
    {
    public:
        typedef float3_X ValueType;
        typedef typename promoteType<float_64, ValueType>::type UnitValueType;
        static const int numComponents = ValueType::dim;

        typedef DataBox<PitchedBox<ValueType, simDim> > DataBoxType;

        typedef MappingDesc::SuperCellSize SuperCellSize;

        FieldB( MappingDesc cellDescription);

        virtual ~FieldB();

        virtual void reset(uint32_t currentStep);

        HDINLINE static UnitValueType getUnit();

        static std::string getName();

        static uint32_t getCommTag();

        virtual EventTask asyncCommunication(EventTask serialEvent);

        void init(FieldE &fieldE, LaserPhysics &laserPhysics);

        DataBoxType getHostDataBox();

        GridLayout<simDim> getGridLayout();

        DataBoxType getDeviceDataBox();

        GridBuffer<ValueType, simDim> &getGridBuffer();

        SimulationDataId getUniqueId();

        void synchronize();

        void syncToDevice();

    private:

        void absorbeBorder();

        void laserManipulation(uint32_t currentStep);

        GridBuffer<ValueType, simDim> *fieldB;

        FieldE *fieldE;
        LaserPhysics *laser;
    };


}

#include "fields/FieldB.tpp"
