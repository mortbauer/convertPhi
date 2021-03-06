/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright held by original author
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

Description
    COnvert volumetric to mass flux

from: http://www.cfd-online.com/Forums/openfoam-solving/99346-quick-way-change-incompressible-compressible.html

\*---------------------------------------------------------------------------*/

#include "argList.H"
#include "fvCFD.H"
#include "fvIOoptionList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    argList::validOptions.insert("rhoRef", "scalar");
    argList::validOptions.insert("poffset", "scalar");
    argList::addBoolOption("inverse", "convert back to incompressible");

    // Add explicit time option
    timeSelector::addOptions();

#   include "setRootCase.H"

    if (!args.options().found("rhoRef"))
    {
        FatalErrorIn(args.executable())
            << "Missing reference density"
            << endl;

        FatalError.exit();
    }


    dimensionedScalar rhoRef
    (
        "rhoRef",
        dimDensity,
        readScalar(IStringStream(args.options()["rhoRef"])())
    );

    Info<< "Reference density = " << rhoRef.value() << endl;

    dimensionedScalar poffset
    (
        "poffset",
        dimPressure,
        args.options().found("poffset") 
        ? readScalar(IStringStream(args.options()["poffset"])())
        : 0
    );


    Info<< "pressure offset = " << poffset.value() << endl;

#   include "createTime.H"
     // Optionally override controlDict time with -time options
    instantList times = timeSelector::selectIfPresent(runTime, args);
    if (times.size() < 1)
    {
        FatalErrorIn(args.executable())
            << "No times selected." << exit(FatalError);
    }
    runTime.setTime(times[0], 0);
    word instance = args.optionLookupOrDefault("instance", runTime.timeName());

#   include "createMesh.H"

    Info<< "Time = " << runTime.value() << endl;

    Info<< "    Reading p" << endl;
    volScalarField p
    (
        IOobject
        (
            "p",
            runTime.timeName(),
            mesh,
            IOobject::MUST_READ,
            IOobject::NO_WRITE
        ),
        mesh
    );

    if ((p.dimensions() == dimPressure)&&(!args.options().found("inverse")))
    {
        Info<< "Pressure " << p.name() << " is a dynamic pressure.  "
            << "Nothing to do"
            << endl;
    }
    else if ((p.dimensions() == dimPressure)&&(args.options().found("inverse")))
    {
        volScalarField rhoP
        (
            IOobject
            (
                "rho" + p.name(),
                runTime.timeName(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            (p-poffset)/rhoRef
        );

        Info<< "Correcting dynamic pressure " << p.name()
            << " into kinematic pressure " << rhoP.name()
            << endl;

        rhoP.write();

    }
    else if (p.dimensions() == dimPressure/dimDensity)
    {
        volScalarField rhoP
        (
            IOobject
            (
                "rho" + p.name(),
                runTime.timeName(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            rhoRef*p+poffset
        );

        Info<< "Correcting kinematic pressure " << p.name()
            << " into dynamic pressure " << rhoP.name()
            << endl;

        rhoP.write();
    }
    else
    {
        Info<< "Cannot recognise dimensions of pressure field " << p.name()
            << ": " << p.dimensions() << ".  Ignoring" << endl;

    }

    Info<< "    Reading phi" << endl;
    surfaceScalarField phi
    (
        IOobject
        (
            "phi",
            runTime.timeName(),
            mesh,
            IOobject::MUST_READ,
            IOobject::NO_WRITE
        ),
        mesh
    );

    if ((phi.dimensions() == dimMass/dimTime)&&((!args.options().found("inverse"))))
    {
        Info<< "Flux " << phi.name() << " is a mass flux.  Nothing to do"
            << endl;
    }
    else if ((phi.dimensions() == dimMass/dimTime)&&((args.options().found("inverse"))))
    {
        surfaceScalarField rhoPhi
        (
            IOobject
            (
                "rho" + phi.name(),
                runTime.timeName(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            phi/rhoRef
        );

        Info<< "Correcting mass flux " << phi.name()
            << " into volume flux " << rhoPhi.name()
            << endl;

        rhoPhi.write();
    }
    else if (phi.dimensions() == dimVolume/dimTime)
    {
        surfaceScalarField rhoPhi
        (
            IOobject
            (
                "rho" + phi.name(),
                runTime.timeName(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            rhoRef*phi
        );

        Info<< "Correcting volume flux " << phi.name()
            << " into mass flux " << rhoPhi.name()
            << endl;

        rhoPhi.write();
    }
    else
    {
        Info<< "Cannot recognise dimensions of flux field " << phi.name()
            << ": " << phi.dimensions() << ".  Ignoring" << endl;
    }

    Info<< "End\n" << endl;

    return(0);
}


// ************************************************************************* //
