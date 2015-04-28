/**
 * Copyright 2013-2015 Anshuman Goswami
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


#include "types.h"
#include "simulation_types.hpp"
#include "plugins/evpath/EVPathWriter.def"

#include "plugins/ISimulationPlugin.hpp"
#include <boost/mpl/vector.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/find.hpp>
#include "compileTime/conversion/MakeSeq.hpp"

#include <boost/type_traits.hpp>

#include "plugins/output/WriteSpeciesCommon.hpp"
#include "plugins/kernel/CopySpecies.kernel"
#include "mappings/kernel/AreaMapping.hpp"

#include "plugins/evpath/writer/ParticleAttribute.hpp"
#include "compileTime/conversion/RemoveFromSeq.hpp"
#include "particles/ParticleDescription.hpp"

namespace picongpu
{

namespace evpath
{
using namespace PMacc;



/** Write copy particle to host memory and dump to EVPath transport
 *
 * @tparam T_Species type of species
 *
 */
template< typename T_Species >
struct WriteSpecies
{
public:

    typedef T_Species ThisSpecies;
    typedef typename ThisSpecies::FrameType FrameType;
    typedef typename FrameType::ParticleDescription ParticleDescription;
    typedef typename FrameType::ValueTypeSeq ParticleAttributeList;


    /* delete multiMask and localCellIdx in evpath particle*/
    typedef bmpl::vector<multiMask,localCellIdx> TypesToDelete;
    typedef typename RemoveFromSeq<ParticleAttributeList, TypesToDelete>::type ParticleCleanedAttributeList;

    /* add globalCellIdx for evpath particle*/
    typedef typename MakeSeq<
            ParticleCleanedAttributeList,
            globalCellIdx<globalCellIdx_pic>
    >::type ParticleNewAttributeList;

    typedef
    typename ReplaceValueTypeSeq<ParticleDescription, ParticleNewAttributeList>::type
    NewParticleDescription;

    typedef Frame<OperatorCreateVectorBox, NewParticleDescription> EVPathFrameType;

    template<typename Space>
    HINLINE void operator()(ThreadParams* params,
                            const Space particleOffset)
    {
        log<picLog::INPUT_OUTPUT > ("EVPath: (begin) write species: %1%") % EVPathFrameType::getName();
        DataConnector &dc = Environment<>::get().DataConnector();
        /* load particle without copy particle data to host */
        ThisSpecies* speciesTmp = &(dc.getData<ThisSpecies >(ThisSpecies::FrameType::getName(), true));

        /* count total number of particles on the device */
        uint64_cu totalNumParticles = 0;

        log<picLog::INPUT_OUTPUT > ("EVPath:  (begin) count particles: %1%") % EVPathFrameType::getName();
        totalNumParticles = PMacc::CountParticles::countOnDevice < CORE + BORDER > (
                                                                                    *speciesTmp,
                                                                                    *(params->cellDescription),
                                                                                    params->localWindowToDomainOffset,
                                                                                    params->window.localDimensions.size);


        log<picLog::INPUT_OUTPUT > ("EVPath:  ( end ) count particles: %1% = %2%") % EVPathFrameType::getName() % totalNumParticles;
        EVPathFrameType hostFrame;
        log<picLog::INPUT_OUTPUT > ("EVPath:  (begin) malloc mapped memory: %1%") % EVPathFrameType::getName();
        /*malloc mapped memory*/
        ForEach<typename EVPathFrameType::ValueTypeSeq, MallocMemory<bmpl::_1> > mallocMem;
        mallocMem(forward(hostFrame), totalNumParticles);
        log<picLog::INPUT_OUTPUT > ("EVPath:  ( end ) malloc mapped memory: %1%") % EVPathFrameType::getName();

        if (totalNumParticles > 0)
        {

            log<picLog::INPUT_OUTPUT > ("EVPath:  (begin) get mapped memory device pointer: %1%") % EVPathFrameType::getName();
            /*load device pointer of mapped memory*/
            EVPathFrameType deviceFrame;
            ForEach<typename EVPathFrameType::ValueTypeSeq, GetDevicePtr<bmpl::_1> > getDevicePtr;
            getDevicePtr(forward(deviceFrame), forward(hostFrame));
            log<picLog::INPUT_OUTPUT > ("EVPath:  ( end ) get mapped memory device pointer: %1%") % EVPathFrameType::getName();

            log<picLog::INPUT_OUTPUT > ("EVPath:  (begin) copy particle to host: %1%") % EVPathFrameType::getName();
            typedef bmpl::vector< typename GetPositionFilter<simDim>::type > usedFilters;
            typedef typename FilterFactory<usedFilters>::FilterType MyParticleFilter;
            MyParticleFilter filter;
            /* activate filter pipeline if moving window is activated */
            filter.setStatus(MovingWindow::getInstance().isSlidingWindowActive());
            filter.setWindowPosition(params->localWindowToDomainOffset,
                                     params->window.localDimensions.size);

            dim3 block(PMacc::math::CT::volume<SuperCellSize>::type::value);

            GridBuffer<int, DIM1> counterBuffer(DataSpace<DIM1>(1));
            AreaMapping < CORE + BORDER, MappingDesc > mapper(*(params->cellDescription));

            __cudaKernel(copySpecies)
                (mapper.getGridDim(), block)
                (counterBuffer.getDeviceBuffer().getPointer(),
                 deviceFrame, speciesTmp->getDeviceParticlesBox(),
                 filter,
                 particleOffset, /*relative to data domain (not to physical domain)*/
                 mapper
                 );
            counterBuffer.deviceToHost();
            log<picLog::INPUT_OUTPUT > ("EVPath:  ( end ) copy particle to host: %1%") % EVPathFrameType::getName();
            __getTransactionEvent().waitForFinished();
            log<picLog::INPUT_OUTPUT > ("EVPath:  all events are finished: %1%") % EVPathFrameType::getName();
            /*this cost a little bit of time but evpath writing is slower^^*/
            assert((uint64_cu) counterBuffer.getHostBuffer().getDataBox()[0] == totalNumParticles);
        }
        /*dump to evpath transport*/
        ForEach<typename EVPathFrameType::ValueTypeSeq, evpath::ParticleAttribute<bmpl::_1> > writeToEVPath;

        writeToEVPath(params, forward(hostFrame), totalNumParticles);

        /*write species counter table to evpath file*/
        log<picLog::INPUT_OUTPUT > ("EVPath:  (begin) writing particle index table for %1%") % EVPathFrameType::getName();
        {
            GridController<simDim>& gc = Environment<simDim>::get().GridController();

            const size_t pos_offset = 2;

            /* particlesMetaInfo = (num particles, scalar position, particle offset x, y, z) */
            uint64_t particlesMetaInfo[5] = {totalNumParticles, gc.getScalarPosition(), 0, 0, 0};
            for (size_t d = 0; d < simDim; ++d)
                particlesMetaInfo[pos_offset + d] = particleOffset[d];

            /* prevent that top (y) gpus have negative value here */
            if (gc.getPosition().y() == 0)
                particlesMetaInfo[pos_offset + 1] = 0;

            if (particleOffset[1] < 0) // 1 == y
                particlesMetaInfo[pos_offset + 1] = 0;

            //int64_t adiosIndexVarId = *(params->adiosSpeciesIndexVarIds.begin());
            //params->adiosSpeciesIndexVarIds.pop_front();
            //ADIOS_CMD(adios_write_byid(params->adiosFileHandle, adiosIndexVarId, particlesMetaInfo));
        }
        log<picLog::INPUT_OUTPUT > ("EVPath:  ( end ) writing particle index table for %1%") % EVPathFrameType::getName();

        /*free host memory*/
        ForEach<typename EVPathFrameType::ValueTypeSeq, FreeMemory<bmpl::_1> > freeMem;
        freeMem(forward(hostFrame));
        log<picLog::INPUT_OUTPUT > ("EVPath: ( end ) writing species: %1%") % EVPathFrameType::getName();
    }
};


} //namspace evpath

} //namespace picongpu
