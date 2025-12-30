# 2025-12-22T17:01:54.423273500
import vitis

client = vitis.create_client()
client.set_workspace(path="fitness_function")

comp = client.get_component(name="fitness_hls")
comp.run(operation="C_SIMULATION")

comp.run(operation="C_SIMULATION")

comp.run(operation="C_SIMULATION")

comp.run(operation="C_SIMULATION")

comp.run(operation="SYNTHESIS")

comp.run(operation="C_SIMULATION")

vitis.dispose()

