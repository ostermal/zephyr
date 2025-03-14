.. zephyr:code-sample:: ipm-net-emul
   :name: IPM on native simulator
   :relevant-api: ipm_interface

   Implement inter-processor mailbox (IPM) on the native simulator using host networing

Overview
********

Overview of the module

Requirements
************

- :zephyr:board:`native_sim` board

Building the application for native_sim
********************************************************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/ipm/ipm_net_emul
   :board: native_sim
   :goals: debug
   :west-args: --sysbuild

Running
*******

Describe
