# 2025-12-22T14:50:05.057255800
import vitis

client = vitis.create_client()
client.set_workspace(path="fitness_function")

comp = client.get_component(name="fitness_hls")
comp.run(operation="C_SIMULATION")

comp.run(operation="C_SIMULATION")

comp.run(operation="C_SIMULATION")

comp.run(operation="C_SIMULATION")

vitis.dispose()

