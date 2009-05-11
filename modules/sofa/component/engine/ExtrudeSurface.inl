/******************************************************************************
*       SOFA, Simulation Open-Framework Architecture, version 1.0 beta 4      *
*                (c) 2006-2009 MGH, INRIA, USTL, UJF, CNRS                    *
*                                                                             *
* This library is free software; you can redistribute it and/or modify it     *
* under the terms of the GNU Lesser General Public License as published by    *
* the Free Software Foundation; either version 2.1 of the License, or (at     *
* your option) any later version.                                             *
*                                                                             *
* This library is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License *
* for more details.                                                           *
*                                                                             *
* You should have received a copy of the GNU Lesser General Public License    *
* along with this library; if not, write to the Free Software Foundation,     *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.          *
*******************************************************************************
*                               SOFA :: Modules                               *
*                                                                             *
* Authors: The SOFA Team and external contributors (see Authors.txt)          *
*                                                                             *
* Contact information: contact@sofa-framework.org                             *
******************************************************************************/
#ifndef SOFA_COMPONENT_ENGINE_EXTRUDESURFACE_INL
#define SOFA_COMPONENT_ENGINE_EXTRUDESURFACE_INL

#if !defined(__GNUC__) || (__GNUC__ > 3 || (_GNUC__ == 3 && __GNUC_MINOR__ > 3))
#pragma once
#endif

#include <sofa/component/engine/ExtrudeSurface.h>
#include <sofa/helper/gl/template.h>
#include <sofa/helper/gl/BasicShapes.h>

namespace sofa
{

namespace component
{

namespace engine
{

using namespace sofa::helper;
using namespace sofa::defaulttype;
using namespace core::objectmodel;

template <class DataTypes>
ExtrudeSurface<DataTypes>::ExtrudeSurface()
    : f_extrusionVertices( initData (&f_extrusionVertices, "extrusionVertices", "Position coordinates of the extrusion") )
    , f_surfaceVertices( initData (&f_surfaceVertices, "surfaceVertices", "Position coordinates of the surface") )
    , f_extrusionTriangles( initData (&f_extrusionTriangles, "extrusionTriangles", "Triangles indices of the extrusion") )
    , f_surfaceTriangles( initData (&f_surfaceTriangles, "surfaceTriangles", "Indices of the triangles of the surface to extrude") )
{
    addInput(&f_surfaceTriangles);
    addInput(&f_surfaceVertices);

    addOutput(&f_extrusionVertices);
    addOutput(&f_extrusionTriangles);
}

template <class DataTypes>
void ExtrudeSurface<DataTypes>::init()
{
    BaseMeshTopology* topology = dynamic_cast<BaseMeshTopology*>(getContext()->getTopology());
    if (topology != NULL)
    {
        BaseData* parent = topology->findField("triangles");
        if (parent == NULL)
        {
            sout << "ERROR: Topology " << topology->getName() << " does not contain triangles" << sendl;
        }
    }
    else
    {
        sout << "ERROR: Topology not found. Extrusion can not be computed" << sendl;
    }

    if (!f_surfaceVertices.isSet() || !f_surfaceTriangles.isSet())
    {
        sout << "ERROR: No indices or vertices given for extrusion" << sendl;
        return;
    }

}

template <class DataTypes>
void ExtrudeSurface<DataTypes>::reinit()
{
    update();
}

template <class DataTypes>
void ExtrudeSurface<DataTypes>::update()
{
    dirty = false;

    BaseMeshTopology* topology = dynamic_cast<BaseMeshTopology*>(getContext()->getTopology());

    VecCoord* extrusionVertices = f_extrusionVertices.beginEdit();
    extrusionVertices->clear();
    helper::vector<BaseMeshTopology::Triangle>* extrusionTriangles = f_extrusionTriangles.beginEdit();
    extrusionTriangles->clear();

    const helper::vector<BaseMeshTopology::TriangleID>& surfaceTriangles = f_surfaceTriangles.getValue();
    const VecCoord& surfaceVertices = f_surfaceVertices.getValue();

    helper::vector<BaseMeshTopology::TriangleID>::const_iterator itTriangles, itTrianglesSide;

    std::map<int, int> pointMatching;
    std::map<BaseMeshTopology::Edge, bool > edgesOnBorder;
    std::set<int> pointsUsed;

    for (itTriangles=surfaceTriangles.begin() ; itTriangles != surfaceTriangles.end() ; itTriangles++)
    {
        BaseMeshTopology::Triangle triangle = topology->getTriangle(*itTriangles);

        VecCoord triangleCoord;

        BaseMeshTopology::Triangle t1, t2;
        //fetch real coords
        for (unsigned int i=0 ; i<3 ; i++)
            triangleCoord.push_back(surfaceVertices[triangle[i]]);

        //compute normal
        Coord n =  cross(triangleCoord[1]-triangleCoord[0], triangleCoord[2]-triangleCoord[0]);

        //create triangle from surface and the new triangle
        //vertex created from surface has an even (2*n) index
        //vertex created from the addition with the normal has an odd (2*n + 1) index
        //a table is also used to map old vertex indices with the new set of indices
        for (unsigned int i=0 ; i<3 ; i++)
        {
            if (pointMatching.find(triangle[i]) == pointMatching.end())
            {
                extrusionVertices->push_back(surfaceVertices[triangle[i]]);
                extrusionVertices->push_back(surfaceVertices[triangle[i]] + n);

                pointMatching[triangle[i]] = extrusionVertices->size() - 2;

                t1[i] = extrusionVertices->size()-2;
                t2[i] = extrusionVertices->size()-1;
            }
            else
            {
                t1[i] = pointMatching[triangle[i]];
                t2[i] = pointMatching[triangle[i]] + 1;
            }
        }
        extrusionTriangles->push_back(t1);
        extrusionTriangles->push_back(t2);

        //to get borders, we simply stock the edge and look if it is already in the table

        BaseMeshTopology::Edge e[3];
        if (t1[0] < t1[1])
        { e[0][0] = t1[0] ; e[0][1] = t1[1] ; }
        else
        { e[0][0] = t1[1] ; e[0][1] = t1[0] ; }

        if (t1[0] < t1[2])
        { e[1][0] = t1[0] ; e[1][1] = t1[2] ; }
        else
        { e[1][0] = t1[2] ; e[1][1] = t1[0] ; }

        if (t1[1] < t1[2])
        { e[2][0] = t1[1] ; e[2][1] = t1[2] ; }
        else
        { e[2][1] = t1[1] ; e[2][0] = t1[2] ; }

        for (unsigned int i=0 ; i<3 ; i++)
        {
            if ( edgesOnBorder.find(e[i])  == edgesOnBorder.end())
                edgesOnBorder[e[i]] = true;
            else
                edgesOnBorder[e[i]] = false;
        }
    }

    std::map<BaseMeshTopology::Edge, bool >::const_iterator itEdges;
    for (itEdges = edgesOnBorder.begin() ; itEdges != edgesOnBorder.end() ; itEdges++)
    {
        //for each edge, we can get the "mirrored one" and construct 2 other triangles
        if ((*itEdges).second)
        {
            BaseMeshTopology::Edge e = (*itEdges).first;

            //first triangle
            BaseMeshTopology::Triangle ft1;
            ft1[0] = e[0];
            ft1[1] = e[1];
            ft1[2] = e[0] + 1;

            //second triangle
            BaseMeshTopology::Triangle ft2;
            ft2[0] = e[0] + 1;
            ft2[1] = e[1] + 1;
            ft2[2] = e[1];

            extrusionTriangles->push_back(ft1);
            extrusionTriangles->push_back(ft2);
        }
    }


    std::cout << extrusionTriangles->size() << std::endl;

    f_extrusionTriangles.endEdit();
    f_extrusionVertices.endEdit();
}

template <class DataTypes>
void ExtrudeSurface<DataTypes>::draw()
{

    const helper::vector<BaseMeshTopology::TriangleID> &surfaceTriangles = f_surfaceTriangles.getValue();

    helper::vector<BaseMeshTopology::TriangleID>::const_iterator itTriangles;
    glDisable(GL_LIGHTING);

    if (!this->getContext()->getShowBehaviorModels())
        return;

    if (this->getContext()->getShowWireFrame())
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    const helper::vector<BaseMeshTopology::Triangle> &extrusionTriangles = f_extrusionTriangles.getValue();
    const VecCoord& extrusionVertices = f_extrusionVertices.getValue();
    helper::vector<BaseMeshTopology::Triangle>::const_iterator it;

    //Triangles From Surface

    glColor3f(1.0,0.0,0.0);
    glBegin(GL_TRIANGLES);
    for (unsigned int i=0 ; i<surfaceTriangles.size()*2 ; i+=2)
    {
        BaseMeshTopology::Triangle triangle = extrusionTriangles[i];

        for (unsigned int j=0 ; j<3 ; j++)
        {
            Coord p = (extrusionVertices[triangle[j]]);
            glVertex3f(p[0], p[1], p[2]);

        }
    }
    glEnd();

    //Triangles From Extrusion
    glColor3f(0.0,1.0,0.0);
    glBegin(GL_TRIANGLES);
    for (unsigned int i=1 ; i<surfaceTriangles.size()*2 + 1 ; i+=2)
    {
        BaseMeshTopology::Triangle triangle = extrusionTriangles[i];

        for (unsigned int j=0 ; j<3 ; j++)
        {
            Coord p = (extrusionVertices[triangle[j]]);
            glVertex3f(p[0], p[1], p[2]);

        }
    }
    glEnd();

    //Border Triangles
    glColor3f(0.0,0.0,1.0);
    glBegin(GL_TRIANGLES);
    for (unsigned int i=surfaceTriangles.size()*2 + 1 ; i<extrusionTriangles.size() ; i++)
    {
        BaseMeshTopology::Triangle triangle = extrusionTriangles[i];

        for (unsigned int j=0 ; j<3 ; j++)
        {
            Coord p = (extrusionVertices[triangle[j]]);
            glVertex3f(p[0], p[1], p[2]);

        }
    }
    glEnd();

    if (this->getContext()->getShowWireFrame())
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glEnable(GL_LIGHTING);
}

} // namespace engine

} // namespace component

} // namespace sofa

#endif
