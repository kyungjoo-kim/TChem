/* =====================================================================================
TChem version 2.0
Copyright (2020) NTESS
https://github.com/sandialabs/TChem

Copyright 2020 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
Under the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
certain rights in this software.

This file is part of TChem. TChem is open source software: you can redistribute it
and/or modify it under the terms of BSD 2-Clause License
(https://opensource.org/licenses/BSD-2-Clause). A copy of the licese is also
provided under the main directory

Questions? Contact Cosmin Safta at <csafta@sandia.gov>, or
           Kyungjoo Kim at <kyukim@sandia.gov>, or
           Oscar Diaz-Ibarra at <odiazib@sandia.gov>

Sandia National Laboratories, Livermore, CA, USA
===================================================================================== */
#include "TChem_CommandLineParser.hpp"
#include "TChem_KineticModelData.hpp"
#include "TChem_Util.hpp"
#include "TChem_EnthalpyMass.hpp"
#include "TChem_TransientContStirredTankReactor.hpp"

using ordinal_type = TChem::ordinal_type;
using real_type = TChem::real_type;
using time_advance_type = TChem::time_advance_type;

using real_type_0d_view = TChem::real_type_0d_view;
using real_type_1d_view = TChem::real_type_1d_view;
using real_type_2d_view = TChem::real_type_2d_view;

using time_advance_type_0d_view = TChem::time_advance_type_0d_view;
using time_advance_type_1d_view = TChem::time_advance_type_1d_view;

using real_type_0d_view_host = TChem::real_type_0d_view_host;
using real_type_1d_view_host = TChem::real_type_1d_view_host;
using real_type_2d_view_host = TChem::real_type_2d_view_host;

using time_advance_type_0d_view_host = TChem::time_advance_type_0d_view_host;
using time_advance_type_1d_view_host = TChem::time_advance_type_1d_view_host;


#define TCHEM_EXAMPLE_SimpleSurface_QOI_PRINT


#if defined(TCHEM_ENABLE_PROBLEM_DAE_CSTR)
#include "TChem_SimpleSurface.hpp"
#include "TChem_InitialCondSurface.hpp"
#endif

int
main(int argc, char* argv[])
{

  /// default inputs
  std::string prefixPath("data/");

  real_type mdotIn(3.596978981250784e-06);
  real_type Vol(0.00013470);
  real_type Acat (0.0013074);


  const real_type zero(0);
  real_type tbeg(0), tend(0.025);
  real_type dtmin(1e-10), dtmax(1e-6);
  real_type rtol_time(1e-4), atol_newton(1e-12), rtol_newton(1e-6);
  int num_time_iterations_per_interval(1e1), max_num_time_iterations(4e3),
    max_num_newton_iterations(100);

  int nBatch(1), team_size(-1), vector_size(-1);
  bool verbose(true);
  int output_frequency(-1);

#if defined(TCHEM_ENABLE_PROBLEM_DAE_CSTR)
  bool transient_initial_condition(false);
  bool initial_condition(true);
#endif
  /// parse command line arguments
  TChem::CommandLineParser opts(
    "This example computes Temperature, density, mass fraction and site "
    "fraction for a plug flow reactor");
  opts.set_option<std::string>(
    "prefixPath", "prefixPath e.g.,inputs/", &prefixPath);

  opts.set_option<real_type>("Acat", "Catalytic area [m2]", &Acat);
  opts.set_option<real_type>("Vol", "Reactor Volumen [m3]", &Vol);
  opts.set_option<real_type>("mdotIn", "Inlet mass flow rate [kg/s]", &mdotIn);

#if defined(TCHEM_ENABLE_PROBLEM_DAE_CSTR)
  opts.set_option<bool>(
    "transient_initial_condition", "If true, use a transient solver to obtain initial condition of the constraint", &transient_initial_condition);
    opts.set_option<bool>(
      "initial_condition", "If true, use a newton solver to obtain initial condition of the constraint", &initial_condition);

#endif

  // opts.set_option<std::string>("chemfile", "Chem file name e.g., chem.inp",
  // &chemFile); opts.set_option<std::string>("thermfile", "Therm file name
  // e.g., therm.dat", &thermFile); opts.set_option<std::string>("chemSurffile",
  // "Chem file name e.g., chem.inp", &chemSurfFile);
  // opts.set_option<std::string>("thermSurffile", "Therm file name e.g.,
  // therm.dat", &thermSurfFile);
  // opts.set_option<std::string>("inputfile", "Input state file name e.g.,
  // input.dat", &inputFile); opts.set_option<std::string>("inputfile", "Input
  // state file name e.g., inputSurfGas.dat", &inputFileSurf);
  // opts.set_option<std::string>("inputfile", "Input state file name e.g.,
  // inputVelocity.dat", &inputFilevelocity);
  // opts.set_option<std::string>("outputfile", "Output rhs file name e.g.,
  // SurfaceRHS.dat", &outputFile);

  opts.set_option<real_type>("tbeg", "Time begin", &tbeg);
  opts.set_option<real_type>("tend", "Time end", &tend);
  opts.set_option<real_type>("dtmin", "Minimum time step size", &dtmin);
  opts.set_option<real_type>("dtmax", "Maximum time step size", &dtmax);
  opts.set_option<real_type>(
    "atol-newton", "Absolute tolerence used in newton solver", &atol_newton);
  opts.set_option<real_type>(
    "rtol-newton", "Relative tolerence used in newton solver", &rtol_newton);
  opts.set_option<real_type>(
    "tol-time", "Tolerence used for adaptive time stepping", &rtol_time);
  opts.set_option<int>("time-iterations-per-interval",
                       "Number of time iterations per interval to store qoi",
                       &num_time_iterations_per_interval);
  opts.set_option<int>("max-time-iterations",
                       "Maximum number of time iterations",
                       &max_num_time_iterations);
  opts.set_option<int>("max-newton-iterations",
                       "Maximum number of newton iterations",
                       &max_num_newton_iterations);
  opts.set_option<int>(
    "batchsize",
    "Batchsize the same state vector described in statefile is cloned",
    &nBatch);
  opts.set_option<bool>(
    "verbose", "If true, printout the first Jacobian values", &verbose);

  opts.set_option<int>(
    "output_frequency", "save data at this iterations", &output_frequency);
  opts.set_option<int>("team-size", "User defined team size", &team_size);
  opts.set_option<int>("vector-size", "User defined vector size", &vector_size);

  const bool r_parse = opts.parse(argc, argv);
  if (r_parse)
    return 0; // print help return

  std::string chemFile(prefixPath + "chem.inp");
  std::string thermFile(prefixPath + "therm.dat");
  std::string chemSurfFile(prefixPath + "chemSurf.inp");
  std::string thermSurfFile(prefixPath + "thermSurf.dat");
  std::string inputFile(prefixPath + "sample.dat");
  std::string inputFileSurf(prefixPath + "inputSurf.dat");

  printf("Inlet mass faction %e\n", mdotIn );
  printf("Catalytic Area %e\n",Acat);
  printf("Vol %e\n",Vol );

  Kokkos::initialize(argc, argv);
  {
    const bool detail = false;

    TChem::exec_space::print_configuration(std::cout, detail);
    TChem::host_exec_space::print_configuration(std::cout, detail);

    const auto exec_space_instance = TChem::exec_space();
    using policy_type =
      typename TChem::UseThisTeamPolicy<TChem::exec_space>::type;

    /// construct kmd and use the view for testing

    TChem::KineticModelData kmdSurf(
      chemFile, thermFile, chemSurfFile, thermSurfFile);
    const auto kmcd =
      kmdSurf
        .createConstData<TChem::exec_space>(); // data struc with gas phase info
    const auto kmcdSurf =
      kmdSurf.createConstSurfData<TChem::exec_space>(); // data struc with
                                                        // surface phase info

    const ordinal_type stateVecDim =
      TChem::Impl::getStateVectorSize(kmcd.nSpec);

    real_type_2d_view_host state_host;
    real_type_2d_view_host siteFraction_host;
    real_type_1d_view_host velocity_host;

    const auto speciesNamesHost = Kokkos::create_mirror_view(kmcd.speciesNames);
    Kokkos::deep_copy(speciesNamesHost, kmcd.speciesNames);

    // get names of species on host
    const auto SurfSpeciesNamesHost =
      Kokkos::create_mirror_view(kmcdSurf.speciesNames);
    Kokkos::deep_copy(SurfSpeciesNamesHost, kmcdSurf.speciesNames);

    {
      // get names of species on host

      // get species molecular weigths
      const auto SpeciesMolecularWeights =
        Kokkos::create_mirror_view(kmcd.sMass);
      Kokkos::deep_copy(SpeciesMolecularWeights, kmcd.sMass);

      TChem::Test::readSample(inputFile,
                              speciesNamesHost,
                              SpeciesMolecularWeights,
                              kmcd.nSpec,
                              stateVecDim,
                              state_host,
                              nBatch);

      TChem::Test::readSurfaceSample(inputFileSurf,
                                     SurfSpeciesNamesHost,
                                     kmcdSurf.nSpec,
                                     siteFraction_host,
                                     nBatch);


      if (state_host.extent(0) != siteFraction_host.extent(0) )
        std::logic_error("Error: number of sample is not valid");
    }

    real_type_2d_view siteFraction("SiteFraction", nBatch, kmcdSurf.nSpec);
    real_type_2d_view state("StateVector ", nBatch, stateVecDim);

    Kokkos::Impl::Timer timer;

#if defined(TCHEM_ENABLE_PROBLEM_DAE_CSTR)
    FILE* fout = fopen("CSTRSolutionDAE.dat", "w");
#else
  FILE* fout = fopen("CSTRSolution.dat", "w");
#endif
    auto writeState =
      [](const ordinal_type iter,
         const real_type_1d_view_host _t,
         const real_type_1d_view_host _dt,
         const real_type_2d_view_host _state,
         const real_type_2d_view_host _siteFraction,
         FILE* fout) { // iteration, time, dt, density, pressure, temperature,
                       // mass fraction, site fraction, velocity
        for (size_t sp = 0; sp < _state.extent(0); sp++) {
          fprintf(fout, "%d \t %15.10e \t  %15.10e \t ", iter, _t(sp), _dt(sp));
          //
          for (ordinal_type k = 0, kend = _state.extent(1); k < kend; ++k)
            fprintf(fout, "%15.10e \t", _state(sp, k));
          //
          for (ordinal_type k = 0, kend = _siteFraction.extent(1); k < kend;
               ++k)
            fprintf(fout, "%15.10e \t", _siteFraction(sp, k));

          fprintf(fout, "\n");
        }

      };


    auto printState = [](const time_advance_type _tadv,
                         const real_type _t,
                         const real_type_1d_view_host _state_at_i,
                         const real_type_1d_view_host _siteFraction_at_i) {
#if defined(TCHEM_EXAMPLE_SimpleSurface_QOI_PRINT)
      /// iter, t, dt, rho, pres, temp, Ys ....
      printf(" Gas \n");
      printf(" t %e dt %e density %e  pres %e temp %e",
             _t,
             _t - _tadv._tbeg,
             _state_at_i(0),
             _state_at_i(1),
             _state_at_i(2));
      printf("\n");
      printf("Mass fraction \n");
      for (ordinal_type k = 3, kend = _state_at_i.extent(0); k < kend; ++k)
        printf(" %e", _state_at_i(k));
      printf("\n");
      printf("Site Fraction \n");
      for (ordinal_type k = 0, kend = _siteFraction_at_i.extent(0); k < kend;
           ++k)
        printf(" %e", _siteFraction_at_i(k));
      printf("\n");
#endif
    };

    timer.reset();
    Kokkos::deep_copy(state, state_host);
    Kokkos::deep_copy(siteFraction, siteFraction_host);

#if defined(TCHEM_ENABLE_PROBLEM_DAE_CSTR)

    if (transient_initial_condition) {

      printf("Running transient initial condition \n" );

      policy_type policy_surf(exec_space_instance, nBatch, Kokkos::AUTO());

      using problem_type_surf =
        Impl::SimpleSurface_Problem<KineticModelConstDataDevice,
                                    KineticSurfModelConstDataDevice>;

      const ordinal_type level = 1;
      const ordinal_type per_team_extent =
        TChem::SimpleSurface::getWorkSpaceSize(kmcd, kmcdSurf);
      ordinal_type per_team_scratch =
        TChem::Scratch<real_type_1d_view>::shmem_size(per_team_extent);
      policy_surf.set_scratch_size(level, Kokkos::PerTeam(per_team_scratch));

        real_type_2d_view fac_surf("fac simple", nBatch, kmcdSurf.nSpec);

      { /// time integration
        real_type_1d_view t_surf("time", nBatch);
        Kokkos::deep_copy(t_surf, 0);
        real_type_1d_view dt_surf("delta time", nBatch);
        Kokkos::deep_copy(dt_surf, 1e-20);

        time_advance_type tadv_default_surf;
        tadv_default_surf._tbeg = 0;
        tadv_default_surf._tend = 1;
        tadv_default_surf._dt = 1e-20;
        tadv_default_surf._dtmin = 1e-20;
        tadv_default_surf._dtmax = 1e-3;
        tadv_default_surf._max_num_newton_iterations = 20;
        tadv_default_surf._num_time_iterations_per_interval = 10;

        time_advance_type_1d_view tadv_surf("tadv simple surface", nBatch);
        Kokkos::deep_copy(tadv_surf, tadv_default_surf);

        real_type_2d_view tol_time_surf(
          "tol time simple surface", problem_type_surf::
          getNumberOfTimeODEs(kmcdSurf), 2);
        real_type_1d_view tol_newton_surf("tol newton simple surface", 2);

        /// tune tolerence
        {
          auto tol_time_host_surf = Kokkos::create_mirror_view(tol_time_surf);
          auto tol_newton_host_surf = Kokkos::create_mirror_view(tol_newton_surf);

          const real_type atol_time_surf = 1e-12;
          const real_type rtol_time_surf = 1e-8;

          const real_type atol_newton_surf = 1e-14;
          const real_type rtol_newton_surf = 1e-8;

          for (ordinal_type i = 0, iend = tol_time_surf.extent(0); i < iend; ++i) {
            tol_time_host_surf(i, 0) = atol_time_surf;
            tol_time_host_surf(i, 1) = rtol_time_surf;
          }
          tol_newton_host_surf(0) = atol_newton_surf;
          tol_newton_host_surf(1) = rtol_newton_surf;

          Kokkos::deep_copy(tol_time_surf, tol_time_host_surf);
          Kokkos::deep_copy(tol_newton_surf, tol_newton_host_surf);
        }

        ordinal_type iter = 0;
        const ordinal_type  max_num_time_iterations_surface(1000);

        real_type tsum(0);
        for (; iter < max_num_time_iterations_surface && tsum <= tend; ++iter) {
          TChem::SimpleSurface::runDeviceBatch(policy_surf,
                                               tol_newton_surf,
                                               tol_time_surf,
                                               tadv_surf,
                                               state,
                                               siteFraction,
                                               t_surf,
                                               dt_surf,
                                               siteFraction,
                                               fac_surf,
                                               kmcd,
                                               kmcdSurf);

          /// carry over time and dt computed in this step
          tsum = zero;
          Kokkos::parallel_reduce(
            Kokkos::RangePolicy<TChem::exec_space>(0, nBatch),
            KOKKOS_LAMBDA(const ordinal_type& i, real_type& update) {
              tadv_surf(i)._tbeg = t_surf(i);
              tadv_surf(i)._dt = dt_surf(i);
              update += t_surf(i);
            },
            tsum);
          Kokkos::fence();
          tsum /= nBatch;
        }

      }


    }
    //Run a newton solver to check that constraint is satified.

    if (initial_condition){
      real_type_2d_view facSurf("facSurf", nBatch, kmcdSurf.nSpec);

      /// team policy
      policy_type policy(exec_space_instance, nBatch, Kokkos::AUTO());

      const ordinal_type level = 1;
      const ordinal_type per_team_extent =
        TChem::InitialCondSurface::getWorkSpaceSize(kmcd, kmcdSurf);
      const ordinal_type per_team_scratch =
        TChem::Scratch<real_type_1d_view>::shmem_size(per_team_extent);
      policy.set_scratch_size(level, Kokkos::PerTeam(per_team_scratch));

      // solve initial condition for PFR solution
      TChem::InitialCondSurface::runDeviceBatch(policy,
                                                state,
                                                siteFraction, // input
                                                siteFraction, // output
                                                facSurf,
                                                kmcd,
                                                kmcdSurf);

      printf("Done with initial Condition for DAE system\n");
    }

#endif
    const real_type t_deepcopy = timer.seconds();

    timer.reset();
    {

      {


        real_type_1d_view t("time", nBatch);
        Kokkos::deep_copy(t, tbeg);
        real_type_1d_view dt("delta time", nBatch);
        Kokkos::deep_copy(dt, dtmin);

        real_type_1d_view_host t_host;
        real_type_1d_view_host dt_host;

        if (output_frequency > 0) {
          t_host = real_type_1d_view_host("time host", nBatch);
          dt_host = real_type_1d_view_host("dt host", nBatch);
        }

        using problem_type =
          TChem::Impl::TransientContStirredTankReactor_Problem<decltype(kmcd),
                                               decltype(kmcdSurf),
                                               cstr_data_type>;
        real_type_2d_view tol_time(
          "tol time", problem_type::getNumberOfTimeODEs(kmcd, kmcdSurf), 2);
        real_type_1d_view tol_newton("tol newton", 2);

        real_type_2d_view fac(
          "fac", nBatch, problem_type::getNumberOfEquations(kmcd, kmcdSurf));

        /// tune tolerence
        {
          auto tol_time_host = Kokkos::create_mirror_view(tol_time);
          auto tol_newton_host = Kokkos::create_mirror_view(tol_newton);

          const real_type atol_time = 1e-12;
          for (ordinal_type i = 0, iend = tol_time.extent(0); i < iend; ++i) {
            tol_time_host(i, 0) = atol_time;
            tol_time_host(i, 1) = rtol_time;
          }
          tol_newton_host(0) = atol_newton;
          tol_newton_host(1) = rtol_newton;

          Kokkos::deep_copy(tol_time, tol_time_host);
          Kokkos::deep_copy(tol_newton, tol_newton_host);
        }

        time_advance_type tadv_default;
        tadv_default._tbeg = tbeg;
        tadv_default._tend = tend;
        tadv_default._dt = dtmin;
        tadv_default._dtmin = dtmin;
        tadv_default._dtmax = dtmax;
        tadv_default._max_num_newton_iterations = max_num_newton_iterations;
        tadv_default._num_time_iterations_per_interval =
          num_time_iterations_per_interval;

        time_advance_type_1d_view tadv("tadv", nBatch);
        Kokkos::deep_copy(tadv, tadv_default);

        /// host views to print QOI
        const auto tadv_at_i = Kokkos::subview(tadv, 0);
        const auto t_at_i = Kokkos::subview(t, 0);
        const auto state_at_i = Kokkos::subview(state, 0, Kokkos::ALL());
        const auto siteFraction_at_i =
          Kokkos::subview(siteFraction, 0, Kokkos::ALL());

        auto tadv_at_i_host = Kokkos::create_mirror_view(tadv_at_i);
        auto t_at_i_host = Kokkos::create_mirror_view(t_at_i);
        auto state_at_i_host = Kokkos::create_mirror_view(state_at_i);
        auto siteFraction_at_i_host =
          Kokkos::create_mirror_view(siteFraction_at_i);

        //setting up cstr reactor
        printf("Setting up CSTR reactor\n");
        cstr_data_type cstr;
        cstr.mdotIn = mdotIn; // inlet mass flow kg/s
        cstr.Vol    = Vol; // volumen of reactor m3
        cstr.Acat   = Acat; // Catalytic area m2: chemical active area
        cstr.pressure = state_host(0, 1);
        // cstr.temperature = state_host(0, 2);

        cstr.Yi = real_type_1d_view("Mass fraction at inlet", kmcd.nSpec);
        printf("Reactor residence time [s] %e\n", state_host(0, 0)*cstr.Vol/cstr.mdotIn);

        // work batch = 1
        Kokkos::parallel_for(
          Kokkos::RangePolicy<TChem::exec_space>(0, nBatch),
          KOKKOS_LAMBDA(const ordinal_type& i) {
            //mass fraction
            for (ordinal_type k = 0; k < kmcd.nSpec; k++) {
              cstr.Yi(k) = state(i,k+3);
            }
        });

        {
          real_type_2d_view EnthalpyMass("EnthalpyMass", 1, kmcd.nSpec);
          real_type_1d_view EnthalpyMixMass("EnthalpyMass Mixture", 1);

          const auto exec_space_instance = TChem::exec_space();
          TChem::EnthalpyMass::runDeviceBatch(exec_space_instance,
                                              team_size,
                                              vector_size,
                                              1,
                                              state,
                                              EnthalpyMass,
                                              EnthalpyMixMass,
                                              kmcd);
          cstr.EnthalpyIn = EnthalpyMixMass(0);
        }

        printf("Done Setting up CSTR reactor\n" );

        ordinal_type iter = 0;
        /// print of store QOI for the first sample
#if defined(TCHEM_EXAMPLE_SimpleSurface_QOI_PRINT)
        {
          /// could use cuda streams
          Kokkos::deep_copy(tadv_at_i_host, tadv_at_i);
          Kokkos::deep_copy(t_at_i_host, t_at_i);
          Kokkos::deep_copy(state_at_i_host, state_at_i);
          Kokkos::deep_copy(siteFraction_at_i_host, siteFraction_at_i);
          printState(tadv_at_i_host(),
                     t_at_i_host(),
                     state_at_i_host,
                     siteFraction_at_i_host);
        }
#endif

        if (output_frequency > 0) {

          const ordinal_type indx = iter / output_frequency;
          printf("save at iteration %d indx %d\n", iter, indx);
          // time, sample, state
          // save time and dt

          Kokkos::deep_copy(dt_host, dt);
          Kokkos::deep_copy(t_host, t);

          fprintf(fout, "%s \t %s \t %s \t ", "iter", "t", "dt");
          fprintf(fout,
                  "%s \t %s \t %s \t",
                  "Density[kg/m3]",
                  "Pressure[Pascal]",
                  "Temperature[K]");

          for (ordinal_type k = 0; k < kmcd.nSpec; k++)
            fprintf(fout, "%s \t", &speciesNamesHost(k, 0));
          //
          for (ordinal_type k = 0; k < kmcdSurf.nSpec; k++)
            fprintf(fout, "%s \t", &SurfSpeciesNamesHost(k, 0));


          fprintf(fout, "\n");
          // save initial condition
          Kokkos::deep_copy(siteFraction_host, siteFraction);

          writeState(-1,
                     t_host,
                     dt_host,
                     state_host,
                     siteFraction_host,
                     fout);
        }



        /// team policy
        policy_type policy(exec_space_instance, nBatch, Kokkos::AUTO());

        const ordinal_type level = 1;
        const ordinal_type per_team_extent =
          TChem::TransientContStirredTankReactor::getWorkSpaceSize(kmcd, kmcdSurf, cstr);
        const ordinal_type per_team_scratch =
          TChem::Scratch<real_type_1d_view>::shmem_size(per_team_extent);
        policy.set_scratch_size(level, Kokkos::PerTeam(per_team_scratch));

        real_type tsum(0);
        for (; iter < max_num_time_iterations && tsum <= tend; ++iter) {

          TChem::TransientContStirredTankReactor::runDeviceBatch(policy,
                                                 tol_newton,
                                                 tol_time,
                                                 fac,
                                                 tadv,
                                                 // input
                                                 state,
                                                 siteFraction,
                                                 // output
                                                 t,
                                                 dt,
                                                 state,
                                                 siteFraction,
                                                 // kinetic info
                                                 kmcd,
                                                 kmcdSurf,
                                                 cstr);
          //

          /// print of store QOI for the first sample
#if defined(TCHEM_EXAMPLE_SimpleSurface_QOI_PRINT)
          {
            /// could use cuda streams
            Kokkos::deep_copy(tadv_at_i_host, tadv_at_i);
            Kokkos::deep_copy(t_at_i_host, t_at_i);
            Kokkos::deep_copy(state_at_i_host, state_at_i);
            Kokkos::deep_copy(siteFraction_at_i_host, siteFraction_at_i);
            printState(tadv_at_i_host(),
                       t_at_i_host(),
                       state_at_i_host,
                       siteFraction_at_i_host);

          }
#endif

          //
          if (iter % output_frequency == 0 && output_frequency > 0) {
            const ordinal_type indx = iter / output_frequency;
            printf("save at iteration %d indx %d\n", iter, indx);
            // time, sample, state
            // save time and dt

            Kokkos::deep_copy(dt_host, dt);
            Kokkos::deep_copy(t_host, t);
            Kokkos::deep_copy(state_host, state);
            Kokkos::deep_copy(siteFraction_host, siteFraction);

            writeState(iter,
                       t_host,
                       dt_host,
                       state_host,
                       siteFraction_host,
                       fout);
          }

          /// carry over time and dt computed in this step
          tsum = zero;
          Kokkos::parallel_reduce(
            Kokkos::RangePolicy<TChem::exec_space>(0, nBatch),
            KOKKOS_LAMBDA(const ordinal_type& i, real_type& update) {
              tadv(i)._tbeg = t(i);
              tadv(i)._dt = dt(i);
              update += t(i);
            },
            tsum);
          Kokkos::fence();
          tsum /= nBatch;
        }
      }
    }
    Kokkos::fence(); /// timing purpose
    const real_type t_device_batch = timer.seconds();
    printf("Time   %e [sec] %e [sec/sample]\n",
           t_device_batch,
           t_device_batch / real_type(nBatch));

    fclose(fout);
  }
  Kokkos::finalize();

  return 0;
}
