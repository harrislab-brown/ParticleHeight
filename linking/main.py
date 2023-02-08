import pandas as pd
import trackpy as tp


def main():
    # import the particle position data
    data_file = 'path to particle data csv'
    particle_positions = pd.read_csv(data_file)

    # construct a velocity predictor
    predictor = tp.predict.NearestVelocityPredict()

    # link trajectories
    max_disp = 1.5
    tp.linking.Linker.MAX_SUB_NET_SIZE = 45
    trajectories = predictor.link_df(particle_positions, max_disp,
                                     pos_columns=['x', 'y', 'z'], t_column='frame', memory=0)

    # save the trajectories csv
    trajectories.to_csv(data_file, index=False)


if __name__ == '__main__':
    main()
